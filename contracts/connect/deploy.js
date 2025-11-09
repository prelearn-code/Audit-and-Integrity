const fs = require('fs');
const path = require('path');
const solc = require('solc');
const { ethers } = require('ethers');

// -------------------- 配置区 --------------------
const RPC_URL = 'http://127.0.0.1:8545';
const PRIVATE_KEY_FILE = 'private.key';
const SOL_FILE = 'vdscontract.sol'; // 用相对路径
const OUTPUT_ABI_FILE = 'vdscontract.json';
const OUTPUT_ADDR_FILE = 'vdscontract.txt';
// -------------------------------------------------

async function main() {
    // 1️⃣ 读取私钥
    const privateKey = fs.readFileSync(PRIVATE_KEY_FILE, 'utf8').trim();
    const provider = new ethers.JsonRpcProvider(RPC_URL);
    const wallet = new ethers.Wallet(privateKey, provider);

    // 2️⃣ 读取合约
    if (!fs.existsSync(SOL_FILE)) {
        console.error(`文件 ${SOL_FILE} 不存在`);
        process.exit(1);
    }
    const source = fs.readFileSync(SOL_FILE, 'utf8');

    // 3️⃣ 编译合约
    const input = {
        language: 'Solidity',
        sources: {
            [SOL_FILE]: { content: source }
        },
        settings: {
            outputSelection: {
                '*': {
                    '*': ['abi', 'evm.bytecode']
                }
            }
        }
    };

    const output = JSON.parse(solc.compile(JSON.stringify(input)));

    // 打印编译错误
    if (output.errors) {
        for (const err of output.errors) {
            console.error(err.formattedMessage || err.message);
        }
        // 如果存在严重错误则退出
        if (output.errors.some(e => e.severity === 'error')) {
            process.exit(1);
        }
    }

    // 4️⃣ 获取合约信息
    const contractName = Object.keys(output.contracts[SOL_FILE])[0];
    const abi = output.contracts[SOL_FILE][contractName].abi;
    const bytecode = output.contracts[SOL_FILE][contractName].evm.bytecode.object;

    if (!bytecode) {
        console.error('合约字节码为空，可能编译失败');
        process.exit(1);
    }

    // 5️⃣ 部署
    console.log(`正在部署合约 ${contractName} ...`);
    const factory = new ethers.ContractFactory(abi, bytecode, wallet);
    const contract = await factory.deploy();
    await contract.waitForDeployment();

    const address = await contract.getAddress();
    console.log(`✅ 合约部署完成！`);
    console.log(`地址: ${address}`);

    // 6️⃣ 输出 ABI 和地址
    fs.writeFileSync(OUTPUT_ABI_FILE, JSON.stringify(abi, null, 2));
    fs.writeFileSync(OUTPUT_ADDR_FILE, address);

    console.log(`ABI 已保存到 ${OUTPUT_ABI_FILE}`);
    console.log(`部署地址已保存到 ${OUTPUT_ADDR_FILE}`);
}

main().catch(err => {
    console.error(err);
    process.exit(1);
});
