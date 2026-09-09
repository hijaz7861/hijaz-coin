import fs from "fs";
import { createVM, runTx } from "@ethereumjs/vm";
import { createLegacyTx } from "@ethereumjs/tx";
import {
  createAddressFromPrivateKey,
  createAddressFromString,
  createAccount,
  hexToBytes,
  bytesToHex
} from "@ethereumjs/util";
import { id } from "ethers";

console.log("========================================");
console.log(" HIJAZ COIN — TX LEVEL EVM TEST");
console.log("========================================");

const TEST_KEY_HEX = process.env.HJZ_TEST_PRIVATE_KEY;

if (!TEST_KEY_HEX) {
  throw new Error("HJZ_TEST_PRIVATE_KEY is required");
}

const TEST_KEY = hexToBytes(TEST_KEY_HEX);

const RECEIVER =
  "0x3333333333333333333333333333333333333333";

const bytecode = fs.readFileSync(
  "direct_build/contracts_HijazCoin_sol_HijazCoin.bin",
  "utf8"
).trim();

const abi = JSON.parse(
  fs.readFileSync(
    "direct_build/contracts_HijazCoin_sol_HijazCoin.abi",
    "utf8"
  )
);

function abiFunction(name) {
  const x = abi.find(
    v => v.type === "function" && v.name === name
  );

  if (!x) {
    throw new Error(`ABI function missing: ${name}`);
  }

  return x;
}

function encodeAddress(address) {
  return address
    .toLowerCase()
    .replace(/^0x/, "")
    .padStart(64, "0");
}

function encodeUint256(value) {
  return BigInt(value)
    .toString(16)
    .padStart(64, "0");
}

function encodeFunction(name, args = []) {
  const item = abiFunction(name);

  const types = item.inputs.map(x => x.type);
  const selector =
    id(`${name}(${types.join(",")})`).slice(2, 10);

  let data = "";

  for (let i = 0; i < args.length; i++) {
    if (types[i] === "address") {
      data += encodeAddress(args[i]);
    } else if (types[i] === "uint256") {
      data += encodeUint256(args[i]);
    } else {
      throw new Error(
        `Unsupported ABI type: ${types[i]}`
      );
    }
  }

  return "0x" + selector + data;
}

function readUint256(result) {
  const hex = bytesToHex(result.execResult.returnValue);
  return BigInt(hex.startsWith("0x") ? hex : "0x" + hex);
}

const vm = await createVM();

console.log("");
console.log("[1/7] VM");
console.log("      EthereumJS VM: READY");

const deployer =
  createAddressFromPrivateKey(TEST_KEY);

await vm.stateManager.putAccount(
  deployer,
  createAccount({
    nonce: 0n,
    balance: 100n * 10n ** 18n
  })
);

console.log("      Deployer:", deployer.toString());
console.log("      Balance : 100 ETH");
console.log("      ACCOUNT: PASS");

console.log("");
console.log("[2/7] DEPLOYMENT TRANSACTION");

const constructorArgument =
  encodeAddress(deployer.toString());

const deploymentData =
  "0x" + bytecode + constructorArgument;

const deployTx = createLegacyTx({
  nonce: 0n,
  gasLimit: 8000000n,
  gasPrice: 1000000000n,
  value: 0n,
  data: hexToBytes(deploymentData)
}).sign(TEST_KEY);

console.log("      TX SIGNED: PASS");
console.log("      Executing deployment...");

const deployResult = await runTx(vm, {
  tx: deployTx
});

if (deployResult.execResult?.exceptionError) {
  throw new Error(
    "DEPLOYMENT FAILED: " +
    deployResult.execResult.exceptionError.error
  );
}

const contractAddress =
  deployResult.createdAddress;

if (!contractAddress) {
  throw new Error(
    "Deployment completed but no contract address returned"
  );
}

console.log("      Contract:", contractAddress);
console.log("      DEPLOYMENT: PASS");

console.log("");
console.log("[3/7] CONTRACT STATE");

async function call(data) {
  const result = await vm.evm.runCall({
    to: contractAddress,
    caller: deployer,
    data: hexToBytes(data)
  });

  if (result.execResult?.exceptionError) {
    throw new Error(
      "CALL FAILED: " +
      result.execResult.exceptionError.error
    );
  }

  return result;
}

const nameResult =
  await call(encodeFunction("name"));

const symbolResult =
  await call(encodeFunction("symbol"));

const decimalsResult =
  await call(encodeFunction("decimals"));

const supplyResult =
  await call(encodeFunction("totalSupply"));

const nameHex =
  bytesToHex(nameResult.execResult.returnValue);

const symbolHex =
  bytesToHex(symbolResult.execResult.returnValue);

const decimals =
  readUint256(decimalsResult);

const totalSupply =
  readUint256(supplyResult);

console.log("      name():      PASS");
console.log("      symbol():    PASS");
console.log("      decimals():", decimals.toString());
console.log("      totalSupply:", totalSupply.toString());

console.log("");
console.log("[4/7] HOLDER BALANCE");

const holderBalanceData =
  encodeFunction("balanceOf", [
    deployer.toString()
  ]);

const holderBefore =
  readUint256(await call(holderBalanceData));

console.log(
  "      Holder BEFORE:",
  holderBefore.toString()
);

if (holderBefore !== totalSupply) {
  throw new Error(
    "Initial holder balance != total supply"
  );
}

console.log("      INITIAL BALANCE: PASS");

console.log("");
console.log("[5/7] TRANSFER TRANSACTION");

const amount =
  1000n * 10n ** 18n;

const transferData =
  encodeFunction("transfer", [
    RECEIVER,
    amount
  ]);

const deployerAccount =
  await vm.stateManager.getAccount(deployer);

const transferNonce =
  deployerAccount.nonce;

console.log(
  "      Current account nonce:",
  transferNonce.toString()
);

const transferTx = createLegacyTx({
  nonce: transferNonce,
  gasLimit: 500000n,
  gasPrice: 1000000000n,
  to: contractAddress,
  value: 0n,
  data: hexToBytes(transferData)
}).sign(TEST_KEY);

console.log("      Amount: 1000 HJZ");
console.log("      TX SIGNED: PASS");
console.log("      Executing transfer...");

const transferResult =
  await runTx(vm, {
    tx: transferTx
  });

if (transferResult.execResult?.exceptionError) {
  throw new Error(
    "TRANSFER FAILED: " +
    transferResult.execResult.exceptionError.error
  );
}

console.log("      TRANSFER EXECUTION: PASS");

console.log("");
console.log("[6/7] BALANCE VERIFICATION");

const holderAfter =
  readUint256(await call(holderBalanceData));

const receiverBalanceData =
  encodeFunction("balanceOf", [RECEIVER]);

const receiverAfter =
  readUint256(await call(receiverBalanceData));

console.log(
  "      Holder AFTER :",
  holderAfter.toString()
);

console.log(
  "      Receiver AFTER:",
  receiverAfter.toString()
);

const expectedHolder =
  holderBefore - amount;

if (holderAfter !== expectedHolder) {
  throw new Error(
    "Holder balance calculation mismatch"
  );
}

if (receiverAfter !== amount) {
  throw new Error(
    "Receiver balance mismatch"
  );
}

console.log("      HOLDER UPDATE: PASS");
console.log("      RECEIVER UPDATE: PASS");

console.log("");
console.log("[7/7] FINAL");

console.log("      Contract creation : PASS");
console.log("      Persistent VM state: PASS");
console.log("      ERC20 state       : PASS");
console.log("      Transfer TX       : PASS");
console.log("      Balance mutation  : PASS");

console.log("");
console.log("========================================");
console.log(" HIJAZ COIN TX LEVEL TEST");
console.log(" STATUS: PASS");
console.log("========================================");
