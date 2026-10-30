#pragma once

#include "hijaz/ledger.hpp"

#include <cstdint>
#include <string>

namespace hijaz {

// OpenSSL 3.5+ provider-backed ML-DSA verifier. The caller supplies a suite ID;
// production suite IDs must be assigned by a separately reviewed protocol ADR.
class OpenSslMlDsaVerifier final : public SignatureVerifier {
public:
    OpenSslMlDsaVerifier(std::uint16_t suite_id, std::string algorithm_name);

    [[nodiscard]] bool supports(std::uint16_t authorization_suite) const noexcept override;
    [[nodiscard]] bool verify(
        std::uint16_t authorization_suite,
        const std::vector<std::uint8_t>& public_key,
        const std::vector<std::uint8_t>& message,
        const std::vector<std::uint8_t>& signature) const override;

private:
    std::uint16_t suite_id_;
    std::string algorithm_name_;
    std::size_t public_key_size_;
    std::size_t signature_size_;
};

} // namespace hijaz
