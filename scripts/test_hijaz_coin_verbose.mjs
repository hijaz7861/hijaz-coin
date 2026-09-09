import fs from "fs";
import { createVM } from "@ethereumjs/vm";
import {
  createAccount,
  createAddressFromPrivateKey,
  createAddressFromString
} from "@ethereumjs/util";
import { createLegacyTx } from "@ethereumjs/tx";
import { Interface } from "ethers";

const base =
  "direct_build/contracts_HijazCoin_sol_HijazCoin";

const bin =
  fs.readFileSync(`${base}.bin`, "utf8").trim();

const abi =
  JSON.parse(fs.readFileSync(`${base}.abi`, "utf8"));

const PRIVATE_KEY_HEX = process.env.HJZ_TEST_PRIVATE_KEY;

if (!PRIVATE_KEY_HEX) {
  throw new Error("HJZ_TEST_PRIVATE_KEY is required");
}

const privateKey = Buffer.from(
  PRIVATE_KEY_HEX.replace(/^0x/, ""),
  "hex"
);

const vm = await createVM();

const deployer =
  createAddressFromPrivateKey(privateKey);

const holder = deployer;

const receiver =
  createAddressFromString(
    "0x3333333333333333333333333333333333333333"
  );

const iface = new Interface(abi);

const expectedSupply =
  1_000_000_000n * 10n ** 18n;

const transferAmount =
  1_000n * 10n ** 18n;

function shortAddress(address) {
  const s = address.toString();
  return `${s.slice(0, 10)}...${s.slice(-8)}`;
}

function hr() {
  console.log("----------------------------------------");
}

function showValue(label, value) {
  console.log(`${label}: ${value.toString()}`);
}

console.log("");
console.log("========================================");
console.log(" HIJAZ COIN — LIVE VERBOSE EVM TEST");
console.log("========================================");
console.log("");

console.log("[BOOT] EthereumJS VM initialize ہو رہی ہے...");
console.log("[BOOT] Contract bytecode load ہو رہا ہے...");

if (!bin) {
  throw new Error("BYTECODE EMPTY");
}

console.log("[BOOT] Bytecode: LOADED");
console.log("[BOOT] Bytecode bytes:", bin.length / 2);

await vm.stateManager.putAccount(
  deployer,
  createAccount({
    nonce: 0n,
    balance: 10n ** 20n
  })
);

console.log("");
console.log("[1/6] TEST ACCOUNT");
console.log("      Deployer :", shortAddress(deployer));
console.log("      Holder   :", shortAddress(holder));
console.log("      Receiver :", shortAddress(receiver));
console.log("      Balance  : 100 ETH");
console.log("      STATUS   : READY");

const constructorArg =
  holder.toString().slice(2).padStart(64, "0");

const deploymentData =
  Buffer.from(bin + constructorArg, "hex");

console.log("");
console.log("[2/6] CONTRACT DEPLOYMENT");
console.log("      Contract : Hijaz Coin");
console.log("      Symbol   : HJZ");
console.log("      Constructor argument:");
console.log("      initialHolder =", shortAddress(holder));
console.log("      Deployment bytes:", deploymentData.length);
console.log("      EVM execution شروع...");

const deployTx =
  createLegacyTx({
    nonce: 0n,
    gasLimit: 10_000_000n,
    gasPrice: 1n,
    value: 0n,
    data: deploymentData
  }).sign(privateKey);

const deployResult =
  await vm.runTx({
    tx: deployTx
  });

if (deployResult.execResult.exceptionError) {
  console.log(
    "      ERROR:",
    deployResult.execResult.exceptionError.error
  );
  throw new Error("DEPLOYMENT FAILED");
}

const contractAddress =
  deployResult.createdAddress;

if (!contractAddress) {
  throw new Error("CONTRACT ADDRESS NOT RETURNED");
}

console.log("      EVM execution: COMPLETE");
console.log("      Contract Address:", contractAddress.toString());
console.log(
  "      Gas Used:",
  deployResult.totalGasSpent?.toString() ?? "N/A"
);
console.log("      STATUS: DEPLOYED");

async function callContract(functionName, args = []) {
  const encoded =
    iface.encodeFunctionData(functionName, args);

  const result =
    await vm.evm.runCall({
      caller: deployer,
      origin: deployer,
      to: contractAddress,
      data: Buffer.from(encoded.slice(2), "hex"),
      gasLimit: 5_000_000n,
      value: 0n
    });

  if (result.execResult.exceptionError) {
    throw new Error(
      `${functionName} failed: ` +
      result.execResult.exceptionError.error
    );
  }

  const returned =
    "0x" +
    Buffer.from(
      result.execResult.returnValue
    ).toString("hex");

  return iface.decodeFunctionResult(
    functionName,
    returned
  );
}

hr();

console.log("[3/6] CONTRACT STATE READ");

const name =
  (await callContract("name"))[0];

const symbol =
  (await callContract("symbol"))[0];

const decimals =
  Number((await callContract("decimals"))[0]);

const totalSupply =
  (await callContract("totalSupply"))[0];

const holderBefore =
  (await callContract(
    "balanceOf",
    [holder.toString()]
  ))[0];

const receiverBefore =
  (await callContract(
    "balanceOf",
    [receiver.toString()]
  ))[0];

console.log("      name()        =", name);
console.log("      symbol()      =", symbol);
console.log("      decimals()    =", decimals);
console.log("      totalSupply() =", totalSupply.toString());
console.log("      holder       =", holderBefore.toString());
console.log("      receiver     =", receiverBefore.toString());

if (name !== "Hijaz Coin")
  throw new Error("NAME CHECK FAILED");

if (symbol !== "HJZ")
  throw new Error("SYMBOL CHECK FAILED");

if (decimals !== 18)
  throw new Error("DECIMALS CHECK FAILED");

if (totalSupply !== expectedSupply)
  throw new Error("SUPPLY CHECK FAILED");

if (holderBefore !== expectedSupply)
  throw new Error("HOLDER BALANCE CHECK FAILED");

if (receiverBefore !== 0n)
  throw new Error("RECEIVER BALANCE CHECK FAILED");

console.log("      STATE VERIFICATION: PASS");

hr();

console.log("[4/6] REAL ERC-20 TRANSFER");
console.log("      From   :", shortAddress(holder));
console.log("      To     :", shortAddress(receiver));
console.log("      Amount : 1,000 HJZ");
console.log("      Amount :", transferAmount.toString(), "base units");
console.log("      EVM transaction executing...");

const transferData =
  iface.encodeFunctionData(
    "transfer",
    [
      receiver.toString(),
      transferAmount
    ]
  );

const transferTx =
  createLegacyTx({
    nonce: 1n,
    gasLimit: 500_000n,
    gasPrice: 1n,
    to: contractAddress,
    value: 0n,
    data: Buffer.from(
      transferData.slice(2),
      "hex"
    )
  }).sign(privateKey);

const transferResult =
  await vm.runTx({
    tx: transferTx
  });

if (transferResult.execResult.exceptionError) {
  console.log(
    "      ERROR:",
    transferResult.execResult.exceptionError.error
  );
  throw new Error("TRANSFER FAILED");
}

console.log("      EVM execution: COMPLETE");
console.log(
  "      Gas Used:",
  transferResult.totalGasSpent?.toString() ?? "N/A"
);
console.log("      STATUS: TRANSFER SUCCESS");

hr();

console.log("[5/6] POST-TRANSFER STATE");

const holderAfter =
  (await callContract(
    "balanceOf",
    [holder.toString()]
  ))[0];

const receiverAfter =
  (await callContract(
    "balanceOf",
    [receiver.toString()]
  ))[0];

const supplyAfter =
  (await callContract(
    "totalSupply"
  ))[0];

console.log("");
console.log("      BEFORE → AFTER");
console.log("");
console.log(
  "      Holder  :",
  holderBefore.toString(),
  "→",
  holderAfter.toString()
);

console.log(
  "      Receiver:",
  receiverBefore.toString(),
  "→",
  receiverAfter.toString()
);

console.log(
  "      Supply  :",
  totalSupply.toString(),
  "→",
  supplyAfter.toString()
);

const expectedHolder =
  expectedSupply - transferAmount;

if (holderAfter !== expectedHolder)
  throw new Error("FINAL HOLDER BALANCE FAILED");

if (receiverAfter !== transferAmount)
  throw new Error("FINAL RECEIVER BALANCE FAILED");

if (supplyAfter !== expectedSupply)
  throw new Error("TOTAL SUPPLY CHANGED");

console.log("");
console.log("      Holder balance: PASS");
console.log("      Receiver balance: PASS");
console.log("      Total supply unchanged: PASS");

hr();

console.log("[6/6] FINAL VERIFICATION");
console.log("      Contract deployment : PASS");
console.log("      Contract state      : PASS");
console.log("      ERC-20 transfer     : PASS");
console.log("      Balance accounting  : PASS");
console.log("      Supply conservation : PASS");

console.log("");
console.log("========================================");
console.log(" HJZ LIVE VERBOSE EVM TEST");
console.log(" STATUS: PASS");
console.log("========================================");
