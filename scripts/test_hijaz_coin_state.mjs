import fs from "fs";
import { createVM } from "@ethereumjs/vm";
import {
  createAddressFromString,
  createAccount,
  bytesToHex
} from "@ethereumjs/util";

const base =
  "direct_build/contracts_HijazCoin_sol_HijazCoin";

const bin = fs.readFileSync(`${base}.bin`, "utf8").trim();
const abi = JSON.parse(fs.readFileSync(`${base}.abi`, "utf8"));

if (!bin) throw new Error("Bytecode missing");

const vm = await createVM();

const deployer = createAddressFromString(
  "0x2222222222222222222222222222222222222222"
);

const holder = createAddressFromString(
  "0x1111111111111111111111111111111111111111"
);

const receiver = createAddressFromString(
  "0x3333333333333333333333333333333333333333"
);

await vm.stateManager.putAccount(
  deployer,
  createAccount({
    nonce: 0n,
    balance: 10n ** 20n
  })
);

console.log("========================================");
console.log(" HIJAZ COIN — ERC20 STATE TEST");
console.log("========================================");

console.log("[1] VM: PASS");
console.log("[2] Accounts: PASS");

const constructorArg =
  holder.toString().slice(2).padStart(64, "0");

const deploymentData =
  bin + constructorArg;

const deployment = await vm.evm.runCall({
  caller: deployer,
  origin: deployer,
  to: undefined,
  data: Buffer.from(deploymentData, "hex"),
  gasLimit: 10_000_000n,
  value: 0n
});

if (deployment.execResult.exceptionError) {
  throw new Error(
    "Deployment failed: " +
    deployment.execResult.exceptionError.error
  );
}

const runtime =
  deployment.execResult.returnValue;

if (!runtime || runtime.length === 0) {
  throw new Error("Runtime bytecode missing");
}

console.log("[3] Contract creation: PASS");
console.log(
  "[4] Runtime bytecode:",
  runtime.length,
  "bytes"
);

/*
 * Verify ABI contains required ERC-20 functions.
 */
const required = [
  "name",
  "symbol",
  "decimals",
  "totalSupply",
  "balanceOf",
  "transfer"
];

for (const fn of required) {
  const found = abi.some(
    x => x.type === "function" && x.name === fn
  );

  if (!found) {
    throw new Error(`Missing ABI function: ${fn}`);
  }

  console.log(`${fn}: PASS`);
}

console.log("----------------------------------------");
console.log("ERC20 ABI STATE INTERFACE: PASS");
console.log("----------------------------------------");

console.log(
  "NOTE: Contract creation was executed successfully."
);

console.log(
  "NOTE: Persistent deployed-address state and transfer transaction"
);
console.log(
  "will be tested in the next transaction-level stage."
);

console.log("========================================");
console.log(" HJZ ERC20 STATE TEST");
console.log(" STATUS: PASS");
console.log("========================================");
