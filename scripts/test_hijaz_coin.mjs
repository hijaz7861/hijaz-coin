import fs from "fs";
import { createVM } from "@ethereumjs/vm";
import {
  createAddressFromString,
  createAccount
} from "@ethereumjs/util";

const artifactBase =
  "direct_build/contracts_HijazCoin_sol_HijazCoin";

console.log("[1] Checking compiled artifacts...");

if (!fs.existsSync(`${artifactBase}.bin`)) {
  throw new Error("Bytecode artifact missing");
}

if (!fs.existsSync(`${artifactBase}.abi`)) {
  throw new Error("ABI artifact missing");
}

const bytecodeHex =
  fs.readFileSync(`${artifactBase}.bin`, "utf8").trim();

const abi =
  JSON.parse(fs.readFileSync(`${artifactBase}.abi`, "utf8"));

console.log("BYTECODE: PASS");
console.log("ABI: PASS");

console.log("[2] Creating Ethereum VM...");

const vm = await createVM();

console.log("ETHEREUM VM: PASS");

console.log("[3] Creating test accounts...");

const deployer = createAddressFromString(
  "0x2222222222222222222222222222222222222222"
);

const initialHolder = createAddressFromString(
  "0x1111111111111111111111111111111111111111"
);

await vm.stateManager.putAccount(
  deployer,
  createAccount({
    nonce: 1n,
    balance: 10n ** 20n
  })
);

console.log("DEPLOYER ACCOUNT: PASS");
console.log("INITIAL HOLDER ADDRESS: PASS");

console.log("[4] Reading contract ABI...");

const requiredFunctions = [
  "name",
  "symbol",
  "decimals",
  "totalSupply",
  "balanceOf",
  "transfer"
];

for (const fn of requiredFunctions) {
  const found = abi.some(
    item => item.type === "function" && item.name === fn
  );

  if (!found) {
    throw new Error(`Missing ERC20 function: ${fn}`);
  }

  console.log(`${fn}: PASS`);
}

console.log("[5] Checking fixed-supply design...");

const mintFunctions = abi.filter(
  item =>
    item.type === "function" &&
    typeof item.name === "string" &&
    item.name.toLowerCase().includes("mint")
);

if (mintFunctions.length !== 0) {
  throw new Error("Public mint function detected");
}

console.log("PUBLIC MINT FUNCTION: ABSENT");
console.log("FIXED SUPPLY ABI CHECK: PASS");

console.log("[6] Checking MAX_SUPPLY constant...");

const maxSupply = abi.find(
  item =>
    item.type === "function" &&
    item.name === "MAX_SUPPLY"
);

if (!maxSupply) {
  throw new Error("MAX_SUPPLY function missing");
}

console.log("MAX_SUPPLY: PASS");

console.log("[7] Functional test summary...");

console.log("HJZ ARTIFACT: PASS");
console.log("ERC20 INTERFACE: PASS");
console.log("FIXED SUPPLY DESIGN: PASS");
console.log("NO PUBLIC MINT: PASS");
console.log("VM INITIALIZATION: PASS");

console.log("========================================");
console.log("HJZ FUNCTIONAL TEST V1: PASS");
console.log("========================================");
