/**
 * 部署顺序：
 * 1️⃣ VDSCore
 * 2️⃣ VDSSearch（依赖 VDSCore）
 * 3️⃣ VDSVerification（依赖 VDSCore + VDSSearch）
 */

const fs = require('fs');
const path = require('path');
const solc = require('solc');
const { ethers } = require('ethers');

// -------------------- 配置区 --------------------
const RPC_URL = 'http://127.0.0.1:8546';
const PRIVATE_KEY_FILE = 'private.key';
const SOL_FILE = 'chain_contract.sol'; // ✅ 三合一文件
const OUTPUT_ABI_FILE = 'chain_contract.sol.json';
const OUTPUT_ADDR_FILE = 'chain_contract.sol.txt';
// -------------------------------------------------

async function compileContracts(filePath) {
    const source = fs.readFileSync(filePath, 'utf8');
    const input = {
        language: 'Solidity',
        sources: { [path.basename(filePath)]: { content: source } },
        settings: {
            optimizer: { enabled: true, runs: 200 }, // ✅ 开启优化，防止超24KB
            outputSelection: { '*': { '*': ['abi', 'evm.bytecode'] } }
        }
    };

    const output = JSON.parse(solc.compile(JSON.stringify(input)));

    if (output.errors) {
        for (const err of output.errors) {
            console.error(err.formattedMessage || err.message);
        }
        if (output.errors.some(e => e.severity === 'error')) {
            throw new Error(`❌ 合约 ${filePath} 编译失败`);
        }
    }

    return output.contracts[path.basename(filePath)];
}

async function deployContract(name, abi, bytecode, wallet, args = []) {
    console.log(`🚀 正在部署 ${name} ...`);
    const factory = new ethers.ContractFactory(abi, bytecode, wallet);
    const contract = await factory.deploy(...args);
    await contract.waitForDeployment();
    const address = await contract.getAddress();
    console.log(`✅ ${name} 部署完成，地址: ${address}`);
    return { contract, address };
}

async function main() {
    const privateKey = fs.readFileSync(PRIVATE_KEY_FILE, 'utf8').trim();
    const provider = new ethers.JsonRpcProvider(RPC_URL);
    const wallet = new ethers.Wallet(privateKey, provider);

    console.log(`连接 RPC: ${RPC_URL}`);
    console.log(`部署账户: ${await wallet.getAddress()}`);

    // 1️⃣ 一次性编译
    const compiled = await compileContracts(SOL_FILE);

    // 2️⃣ 提取三个合约
    const VDSCore = compiled['VDSCore'];
    const VDSSearch = compiled['VDSSearch'];
    const VDSVerification = compiled['VDSVerification'];

    // 3️⃣ 部署
    const { address: coreAddr } = await deployContract('VDSCore', VDSCore.abi, VDSCore.evm.bytecode.object, wallet);
    const { address: searchAddr } = await deployContract('VDSSearch', VDSSearch.abi, VDSSearch.evm.bytecode.object, wallet, [coreAddr]);
    const { address: verifyAddr } = await deployContract('VDSVerification', VDSVerification.abi, VDSVerification.evm.bytecode.object, wallet, [coreAddr, searchAddr]);

    // 4️⃣ 输出结果
    const allAbi = {
        VDSCore: VDSCore.abi,
        VDSSearch: VDSSearch.abi,
        VDSVerification: VDSVerification.abi
    };

    fs.writeFileSync(OUTPUT_ABI_FILE, JSON.stringify(allAbi, null, 2));
    fs.writeFileSync(OUTPUT_ADDR_FILE, `VDSCore: ${coreAddr}\nVDSSearch: ${searchAddr}\nVDSVerification: ${verifyAddr}\n`);

    console.log(`\n📦 ABI 已保存到: ${OUTPUT_ABI_FILE}`);
    console.log(`📜 部署地址已保存到: ${OUTPUT_ADDR_FILE}`);
    console.log(`✅ 所有合约部署成功！`);
}

main().catch(err => {
    console.error('❌ 部署失败:', err);
    process.exit(1);
});
