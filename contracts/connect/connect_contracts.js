// test_contracts.js

const fs = require('fs');
const path = require('path');
const { ethers } = require('ethers');

// -------------------- 配置区 --------------------
const RPC_URL = 'http://127.0.0.1:8546'; // 本地 Geth RPC 端点
const ABI_FILE = 'chain_contract.sol.json'; // ABI 文件名 (由部署脚本生成)
const ADDR_FILE = 'chain_contract.sol.txt'; // 部署地址文件名 (由部署脚本生成)
const PRIVATE_KEY_FILE = 'private.key'; // 私钥文件名
// -------------------------------------------------

async function main() {
    try {
        // 1️⃣ 读取配置文件
        console.log('🔍 步骤 1: 读取配置文件...');

        // 读取私钥
        if (!fs.existsSync(PRIVATE_KEY_FILE)) {
            throw new Error(`❌ 私钥文件 ${PRIVATE_KEY_FILE} 不存在！`);
        }
        const privateKey = fs.readFileSync(PRIVATE_KEY_FILE, 'utf8').trim();
        if (!privateKey) {
            throw new Error(`❌ 私钥文件 ${PRIVATE_KEY_FILE} 为空！`);
        }

        // 读取 ABI
        if (!fs.existsSync(ABI_FILE)) {
            throw new Error(`❌ ABI 文件 ${ABI_FILE} 不存在！`);
        }
        const abiData = JSON.parse(fs.readFileSync(ABI_FILE, 'utf8'));

        // 读取合约地址
        if (!fs.existsSync(ADDR_FILE)) {
            throw new Error(`❌ 地址文件 ${ADDR_FILE} 不存在！`);
        }
        const addrContent = fs.readFileSync(ADDR_FILE, 'utf8');
        const addrLines = addrContent.split('\n').filter(line => line.trim() !== '');
        const addresses = {};
        addrLines.forEach(line => {
            const [key, value] = line.split(': ');
            if (key && value) {
                addresses[key.trim()] = value.trim(); // 例如: addresses.VDSCore = "0x..."
            }
        });

        // 验证必要的 ABI 和地址是否存在
        const requiredContracts = ['VDSCore', 'VDSSearch', 'VDSVerification'];
        for (const contractName of requiredContracts) {
            if (!abiData[contractName]) {
                throw new Error(`❌ ABI 文件中缺少 ${contractName} 合约的 ABI！`);
            }
            if (!addresses[contractName]) {
                throw new Error(`❌ 地址文件中缺少 ${contractName} 合约的地址！`);
            }
        }

        console.log('✅ 配置文件读取成功。');

        // 2️⃣ 初始化 Provider 和 Wallet
        console.log('\n🔗 步骤 2: 初始化 Provider 和 Wallet...');
        const provider = new ethers.JsonRpcProvider(RPC_URL);
        const wallet = new ethers.Wallet(privateKey, provider);
        const walletAddress = await wallet.getAddress();
        console.log(`✅ 使用账户: ${walletAddress}`);
        console.log(`✅ 连接到 RPC: ${RPC_URL}`);

        // 3️⃣ 初始化合约实例
        console.log('\n🏗️ 步骤 3: 初始化合约实例...');
        // 重要：VDSVerification 和 VDSSearch 需要引用 VDSCore 和彼此的地址
        // 因此，必须先创建 VDSCore 实例，然后是 VDSSearch，最后是 VDSVerification
        const coreContract = new ethers.Contract(addresses.VDSCore, abiData.VDSCore, wallet);
        const searchContract = new ethers.Contract(addresses.VDSSearch, abiData.VDSSearch, wallet);
        const verificationContract = new ethers.Contract(addresses.VDSVerification, abiData.VDSVerification, wallet);
        console.log('✅ 合约实例初始化完成。');
        console.log(`   VDSCore 合约地址: ${addresses.VDSCore}`);
        console.log(`   VDSSearch 合约地址: ${addresses.VDSSearch}`);
        console.log(`   VDSVerification 合约地址: ${addresses.VDSVerification}`);

        // 4️⃣ 进行交互测试 (只读，检查部署和连接)
        console.log('\n🧪 步骤 4: 开始交互测试 (检查部署与连接)...');

        // --- 测试 VDSCore 合约 ---
        console.log('\n--- 测试 VDSCore ---');
        try {
            // 检查是否已初始化 (这是初始化状态的直接反映)
            const params = await coreContract.getPublicParameters();
            console.log(`✅ VDSCore 初始化状态: ${params.initialized}`);
            if (!params.initialized) {
                console.log("💡 VDSCore 尚未初始化。");
                // 尝试读取系统统计信息 (这些在未初始化时也应返回 0)
                const stats = await coreContract.getSystemStats();
                console.log(`📊 VDSCore 初始统计 (未初始化): 客户端总数=${stats[0]}, 存储节点总数=${stats[1]}, 文件总数=${stats[2]}`);
            } else {
                console.log("💡 VDSCore 已初始化。");
            }
        } catch (error) {
            console.error(`❌ VDSCore 测试出错: ${error.message}`);
        }

        // --- 测试 VDSSearch 合约 ---
        console.log('\n--- 测试 VDSSearch ---');
        try {
            // VDSSearch 依赖 VDSCore 地址，检查其是否正确设置
            const coreAddrFromSearch = await searchContract.coreContract();
            console.log(`✅ VDSSearch 中存储的 VDSCore 地址: ${coreAddrFromSearch}`);
            console.log(`✅ 本地配置的 VDSCore 地址: ${addresses.VDSCore}`);
            console.log(`✅ 地址匹配: ${coreAddrFromSearch.toLowerCase() === addresses.VDSCore.toLowerCase()}`);

            // 尝试读取 VDSSearch 自己的状态 (这些不依赖 VDSCore 初始化)
            const totalSearchRequests = await searchContract.getTotalSearchRequests();
            console.log(`📊 VDSSearch 初始统计 (未初始化 VDSCore): 搜索请求数=${totalSearchRequests}`);

            // 获取所有搜索请求ID (应为空数组)
            const searchIds = await searchContract.getAllSearchRequestIds();
            console.log(`🔍 VDSSearch 当前搜索请求列表长度: ${searchIds.length}`);

        } catch (error) {
            console.error(`❌ VDSSearch 测试出错: ${error.message}`);
        }

        // --- 测试 VDSVerification 合约 ---
        console.log('\n--- 测试 VDSVerification ---');
        try {
            // VDSVerification 依赖 VDSCore 和 VDSSearch 地址，检查它们是否正确设置
            const coreAddrFromVerification = await verificationContract.coreContract();
            const searchAddrFromVerification = await verificationContract.searchContract();
            console.log(`✅ VDSVerification 中存储的 VDSCore 地址: ${coreAddrFromVerification}`);
            console.log(`✅ 本地配置的 VDSCore 地址: ${addresses.VDSCore}`);
            console.log(`✅ VDSVerification 中存储的 VDSSearch 地址: ${searchAddrFromVerification}`);
            console.log(`✅ 本地配置的 VDSSearch 地址: ${addresses.VDSSearch}`);
            const coreAddrMatch = coreAddrFromVerification.toLowerCase() === addresses.VDSCore.toLowerCase();
            const searchAddrMatch = searchAddrFromVerification.toLowerCase() === addresses.VDSSearch.toLowerCase();
            console.log(`✅ VDSCore 地址匹配: ${coreAddrMatch}`);
            console.log(`✅ VDSSearch 地址匹配: ${searchAddrMatch}`);

            if (coreAddrMatch && searchAddrMatch) {
                console.log("✅ VDSVerification 成功连接到 VDSCore 和 VDSSearch。");
            } else {
                 console.log("❌ VDSVerification 地址连接可能存在问题。");
                 // 注意：如果地址不匹配，后续与 VDSCore 或 VDSSearch 的交互会失败
            }

            // 尝试读取 VDSVerification 自己的状态 (这些不依赖 VDSCore 初始化)
            const proofIds = await verificationContract.getAllIntegrityProofIds();
            console.log(`📊 VDSVerification 初始统计 (未初始化 VDSCore): 完整性证明数=${proofIds.length}`);

        } catch (error) {
            console.error(`❌ VDSVerification 测试出错: ${error.message}`);
        }

        console.log('\n🎉 部署与连接测试完成。');
        console.log('\n💡 提示: 系统尚未初始化。请先使用已部署的 VDSCore 合约地址调用 initializeSystem(...) 函数。');

    } catch (error) {
        console.error('\n❌ 主程序执行失败:', error);
        process.exit(1);
    }
}

main();
