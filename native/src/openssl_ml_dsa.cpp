#include "hijaz/openssl_ml_dsa.hpp"

#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/params.h>

#include <memory>
#include <stdexcept>
#include <utility>

namespace hijaz {
namespace {
using PkeyContext = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
using Pkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using Signature = std::unique_ptr<EVP_SIGNATURE, decltype(&EVP_SIGNATURE_free)>;

struct AlgorithmSizes {
    const char* name;
    std::size_t public_key;
    std::size_t signature;
};

constexpr AlgorithmSizes algorithms[] = {
    {"ML-DSA-44", 1312, 2420},
    {"ML-DSA-65", 1952, 3309},
    {"ML-DSA-87", 2592, 4627}
};

const AlgorithmSizes& lookup_algorithm(const std::string& name) {
    for (const auto& algorithm : algorithms) {
        if (name == algorithm.name) return algorithm;
    }
    throw std::invalid_argument("unsupported ML-DSA algorithm name");
}
} // namespace

OpenSslMlDsaVerifier::OpenSslMlDsaVerifier(
    std::uint16_t suite_id, std::string algorithm_name)
    : suite_id_(suite_id),
      algorithm_name_(std::move(algorithm_name)),
      public_key_size_(lookup_algorithm(algorithm_name_).public_key),
      signature_size_(lookup_algorithm(algorithm_name_).signature) {
    if (suite_id_ == 0) throw std::invalid_argument("authorization suite ID zero is reserved");
}

bool OpenSslMlDsaVerifier::supports(std::uint16_t authorization_suite) const noexcept {
    return suite_id_ != 0 && authorization_suite == suite_id_;
}

bool OpenSslMlDsaVerifier::verify(
    std::uint16_t authorization_suite,
    const std::vector<std::uint8_t>& public_key,
    const std::vector<std::uint8_t>& message,
    const std::vector<std::uint8_t>& signature) const {

    if (!supports(authorization_suite) ||
        public_key.size() != public_key_size_ ||
        signature.size() != signature_size_) {
        return false;
    }

    PkeyContext import_context(
        EVP_PKEY_CTX_new_from_name(nullptr, algorithm_name_.c_str(), nullptr),
        EVP_PKEY_CTX_free);
    if (!import_context || EVP_PKEY_fromdata_init(import_context.get()) <= 0)
        return false;

    auto* mutable_public_key = const_cast<std::uint8_t*>(public_key.data());
    OSSL_PARAM key_parameters[] = {
        OSSL_PARAM_construct_octet_string(
            OSSL_PKEY_PARAM_PUB_KEY, mutable_public_key, public_key.size()),
        OSSL_PARAM_construct_end()
    };

    EVP_PKEY* raw_key = nullptr;
    if (EVP_PKEY_fromdata(
            import_context.get(), &raw_key, EVP_PKEY_PUBLIC_KEY, key_parameters) <= 0) {
        EVP_PKEY_free(raw_key);
        return false;
    }
    Pkey key(raw_key, EVP_PKEY_free);

    Signature algorithm(EVP_SIGNATURE_fetch(nullptr, algorithm_name_.c_str(), nullptr),
                        EVP_SIGNATURE_free);
    if (!algorithm) return false;

    PkeyContext verify_context(EVP_PKEY_CTX_new_from_pkey(nullptr, key.get(), nullptr),
                               EVP_PKEY_CTX_free);
    if (!verify_context ||
        EVP_PKEY_verify_message_init(verify_context.get(), algorithm.get(), nullptr) <= 0) {
        return false;
    }

    return EVP_PKEY_verify(
        verify_context.get(), signature.data(), signature.size(),
        message.data(), message.size()) == 1;
}

} // namespace hijaz
