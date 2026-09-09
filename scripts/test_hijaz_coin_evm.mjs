import fs from "fs";
import { createVM } from "@ethereumjs/vm";
import {
  createAddressFromString,
  createAccount
} from "@ethereumjs/util";

const artifactBase =
  "direct_build/contracts_HijazCoin_sol_HijazCoin";

const bytecodeHex =
  fs.readFileSync(`${artifactBase}.bin`, "utf8").trim();

if (!bytecodeHex) {
  throw new Error("Contract bytecode is empty");
}

const vm = await createVM();

const deployer = createAddressFromString(
  "0x2222222222222222222222222222222222222222"
);

const holder = createAddressFromString(
  "0x1111111111111111111111111111111111111111"
);

await vm.stateManager.putAccount(
  deployer,
  createAccount({
    nonce: 0n,
    balance: 10n ** 20n
  })
);

console.log("[1] VM + account setup: PASS");

const constructorArg =
  holder.toString().slice(2).padStart(64, "0");

const deploymentData =
  bytecodeHex + constructorArg;

console.log("[2] Constructor ABI encoding: PASS");
console.log(
  "[3] Deployment bytecode hex length:",
  deploymentData.length
);

/*
 * Contract-creation execution.
 * No network and no real funds are involved.
 */
const result = await vm.evm.runCall({
  caller: deployer,
  origin: deployer,
  to: undefined,
  data: Buffer.from(deploymentData, "hex"),
  gasLimit: 10_000_000n,
  value: 0n
});

console.log("[4] EVM execution completed: PASS");

console.log(
  "[5] Returned bytecode length:",
  result.execResult.returnValue.length
);

console.log(
  "[6] Execution exception:",
  result.execResult.exceptionError
    ? result.execResult.exceptionError.error
    : "NONE"
);

if (result.execResult.exceptionError) {
  throw new Error(
    "EVM deployment execution failed: " +
    result.execResult.exceptionError.error
  );
}

if (result.execResult.returnValue.length === 0) {
  throw new Error(
    "Contract creation returned empty runtime bytecode"
  );
}

console.log("[7] Runtime bytecode returned: PASS");

console.log("========================================");
console.log(" HJZ ACTUAL EVM EXECUTION TEST");
console.log(" STATUS: PASS");
console.log("========================================");
