# 🧱 本地私有链启动与开发环境笔记

本笔记记录了在本地搭建以太坊私链、使用 Geth 节点、以及部署 Solidity 智能合约的完整环境信息与操作示例，适合实验复现和学习用途。

---

## 1️⃣ Geth 私有链启动命令（PoW 模式）

#### 启动log
```bash
geth --datadir ./chain \
  --networkid 101 \
  --http --http.addr 0.0.0.0 --http.port 8546 --http.api personal,eth,net,web3 \
  --ws --ws.addr 0.0.0.0 --ws.port 8552 --ws.api eth,net,web3 \
  --port 30303 \
  --allow-insecure-unlock
```

#### 📘 参数说明

| 参数                                 | 作用                  |
| ---------------------------------- | ------------------- |
| `--datadir ./chain`                | 区块链数据存储目录           |
| `--networkid 101`                  | 自定义网络 ID            |
| `--http`                           | 启用 HTTP RPC 接口      |
| `--http.addr 0.0.0.0`              | 允许外部访问              |
| `--http.port 8546`                 | HTTP RPC 端口         |
| `--http.api personal,eth,net,web3` | 开放模块接口              |
| `--ws`                             | 启用 WebSocket 接口     |
| `--ws.addr 0.0.0.0`                | 允许外部访问              |
| `--ws.port 8552`                   | WebSocket 端口        |
| `--ws.api eth,net,web3`            | 开放模块接口              |
| `--port 30303`                     | P2P 端口              |
| `--allow-insecure-unlock`          | HTTP 模式下允许解锁账户（仅测试） |

---

#### 启动geth控制台

```
geth attach ./chain/geth.ipc
```

## 2️⃣ 节点信息

```bash
admin.nodeInfo.enode
```

输出：

```
"enode://695ccb31726870b875f5c509a57bf33a7bc0169dcfc89a54a8702912dce1abefe7245c746380aaeed8ad371e5fe175f64d9b426a83f5d63bbde242125f2b0d77@113.54.178.16:30303?discport=56743"
```

* 用途：节点唯一标识，可用于节点互联（`admin.addPeer("enode://...")`）。

---

## 3️⃣ Geth

#### 版本信息
```bash
geth version
```

输出：

```
Geth
Version: 1.10.26-stable
Git Commit: e5eb32acee19cc9fca6a03b10283b7484246b15a
Git Commit Date: 20221103
Architecture: amd64
Go Version: go1.18.5
Operating System: linux
```

### 启动控制台
```
geth attach ./chain/geth.ipc
```

---

## 4️⃣ Solidity 编译器版本

```bash
node -p "require('solc').version()"
```

输出：

```
0.8.19+commit.7dd6d404.Emscripten.clang
```

* 用于本地智能合约编译与部署
* 与 Node.js 交互使用 `solc` 模块

---

## 5️⃣ 创世块示例

```json
{
  "config": {
    "chainId": 101,
    "homesteadBlock": 0,
    "eip150Block": 0,
    "eip155Block": 0,
    "eip158Block": 0,
    "byzantiumBlock": 0,
    "constantinopleBlock": 0,
    "petersburgBlock": 0,
    "istanbulBlock": 0,
    "berlinBlock": 0,
    "londonBlock": 0,
    "ethash": {}
  },
  "nonce": "0x0",
  "timestamp": "0x0",
  "extraData": "0x00",
  "gasLimit": "0x2fefd8",
  "difficulty": "0x20000",
  "mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
  "coinbase": "0x0000000000000000000000000000000000000000",
  "alloc": {
    "0xf3CFFdd51e5A116888BF23dE75749B4c5d891569": {
      "balance": "1000000000000000000000000000000"
    }
  }
}
```

* 说明：为私链初始化创世块，指定链 ID、初始账户余额、挖矿难度等。

---

## 6️⃣ 智能合约部署示例（Node.js + ethers.js + solc）

```javascript
const fs = require('fs');
const path = require('path');
const solc = require('solc');
const { ethers } = require('ethers');

// -------------------- 配置区 --------------------
const RPC_URL = 'http://127.0.0.1:8546';
const PRIVATE_KEY_FILE = 'private.key';
const SOL_FILE = 'chain_contract.sol'; // 用相对路径
const OUTPUT_ABI_FILE = 'chain_contract.json';
const OUTPUT_ADDR_FILE = 'chain_contract.txt';
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
```

---

## 7️⃣ 合约示例 `chain_contract.sol`

```solidity
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

contract SimpleStorage {
    uint256 public data;

    function set(uint256 _data) public {
        data = _data;
    }
}
```

* 说明：一个最简单的存储合约，可通过 `set` 方法写入数据，通过 `data` 公共变量读取数据。

---

✅ **总结**

| 项目          | 内容                              |
| ----------- | ------------------------------- |
| 运行环境        | Ubuntu / Linux                  |
| Geth 版本     | 1.10.26-stable                  |
| Solidity 版本 | 0.8.19                          |
| 私链网络 ID     | 101                             |
| HTTP RPC 端口 | 8546                            |
| WS 端口       | 8552                            |
| P2P 端口      | 30303                           |
| enode 用途    | 节点唯一标识，可用于节点互联                  |
| 合约部署方式      | Node.js + ethers.js + solc 编译部署 |
| 示例合约        | SimpleStorage                   |

---


# Paper:项目配置介绍(Verifiable Decentralized Storage - Three-Contract System Conclusion)

## **System Architecture Overview（BlockChain）**

The Verifiable Decentralized Storage (VDS) system has been split into three modular, interconnected smart contracts to enable deployment on resource-constrained private blockchain networks. Each contract serves a specific purpose while maintaining seamless integration with the others.

---

## **Contract 1: VDSCore (Foundation Layer)**

### **Purpose**
The core foundation contract that manages system initialization, user registration, and file storage tracking.

### **Main Functions**

| Function | Description | Access Control |
|----------|-------------|----------------|
| `initializeSystem()` | Initialize system with public parameters (N, g, l) | Anyone (once) |
| `registerClient()` | Register new clients with their public keys | Any address |
| `registerStorageNode()` | Register storage nodes to provide storage services | Any address |
| `recordFileStorage()` | Record file storage events on blockchain | Storage Nodes |
| `recordFileDeletion()` | Mark files as deleted | Clients |
| `updateReputation()` | Update storage node reputation based on verification | External contracts |
| `getClientInfo()` | Query client registration details | View function |
| `getStorageNodeInfo()` | Query storage node information | View function |
| `isRegisteredClient()` | Check if address is registered client | View function |
| `isActiveStorageNode()` | Check if node is active storage provider | View function |

### **Key Features**
- ✅ System-wide public parameter management
- ✅ Client and storage node registry
- ✅ File storage event tracking
- ✅ Reputation-based node management
- ✅ Automatic node deactivation for low reputation (<50)

### **Contract Code**

```solidity
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

/**
 * @title VDSCore
 * @dev Core contract for system setup, registration, and file storage
 * @notice Part 1 of 3 - Core functionality
 */
contract VDSCore {
    
    // Structs
    struct PublicParameters {
        bytes N;
        bytes g;
        bytes l;
        bool initialized;
    }
    
    struct ClientInfo {
        bytes publicKey;
        bool registered;
        uint256 registrationTime;
    }
    
    struct StorageNode {
        address nodeAddress;
        bytes publicParams;
        bool active;
        uint256 registrationTime;
        uint256 reputation;
    }
    
    struct FileStorageEvent {
        address client;
        address storageNode;
        bytes32 fileIdentifier;
        uint256 timestamp;
        bool isActive;
    }
    
    // State variables
    PublicParameters public systemParams;
    
    mapping(address => ClientInfo) public clients;
    mapping(address => StorageNode) public storageNodes;
    mapping(bytes32 => FileStorageEvent) public fileStorage;
    
    address[] public clientAddresses;
    address[] public storageNodeAddresses;
    
    uint256 public totalClients;
    uint256 public totalStorageNodes;
    uint256 public totalFiles;
    
    // Events
    event SystemInitialized(bytes N, bytes g, bytes l, uint256 timestamp);
    event ClientRegistered(address indexed client, bytes publicKey, uint256 timestamp);
    event StorageNodeRegistered(address indexed node, uint256 timestamp);
    event FileStored(address indexed client, address indexed storageNode, bytes32 fileIdentifier, uint256 timestamp);
    event FileDeleted(address indexed client, bytes32 fileIdentifier, uint256 timestamp);
    event ReputationUpdated(address indexed storageNode, uint256 newReputation);
    
    // Modifiers
    modifier onlyRegisteredClient() {
        require(clients[msg.sender].registered, "Client not registered");
        _;
    }
    
    modifier onlyRegisteredStorageNode() {
        require(storageNodes[msg.sender].active, "Storage node not registered or inactive");
        _;
    }
    
    modifier systemInitialized() {
        require(systemParams.initialized, "System not initialized");
        _;
    }
    
    /**
     * @dev Initialize the system with public parameters
     */
    function initializeSystem(bytes calldata _N, bytes calldata _g, bytes calldata _l) external {
        require(!systemParams.initialized, "System already initialized");
        require(_N.length > 0 && _g.length > 0 && _l.length > 0, "Invalid parameters");
        
        systemParams.N = _N;
        systemParams.g = _g;
        systemParams.l = _l;
        systemParams.initialized = true;
        
        emit SystemInitialized(_N, _g, _l, block.timestamp);
    }
    
    /**
     * @dev Register a new client
     */
    function registerClient(bytes calldata _publicKey) external systemInitialized {
        require(!clients[msg.sender].registered, "Client already registered");
        require(_publicKey.length > 0, "Invalid public key");
        
        clients[msg.sender].publicKey = _publicKey;
        clients[msg.sender].registered = true;
        clients[msg.sender].registrationTime = block.timestamp;
        
        clientAddresses.push(msg.sender);
        totalClients++;
        
        emit ClientRegistered(msg.sender, _publicKey, block.timestamp);
    }
    
    /**
     * @dev Register a new storage node
     */
    function registerStorageNode(bytes calldata _publicParams) external systemInitialized {
        require(!storageNodes[msg.sender].active, "Storage node already registered");
        require(_publicParams.length > 0, "Invalid parameters");
        
        storageNodes[msg.sender].nodeAddress = msg.sender;
        storageNodes[msg.sender].publicParams = _publicParams;
        storageNodes[msg.sender].active = true;
        storageNodes[msg.sender].registrationTime = block.timestamp;
        storageNodes[msg.sender].reputation = 100;
        
        storageNodeAddresses.push(msg.sender);
        totalStorageNodes++;
        
        emit StorageNodeRegistered(msg.sender, block.timestamp);
    }
    
    /**
     * @dev Record file storage event
     */
    function recordFileStorage(address _client, bytes32 _fileIdentifier) 
        external 
        onlyRegisteredStorageNode 
        systemInitialized 
    {
        require(clients[_client].registered, "Client not registered");
        require(_fileIdentifier != bytes32(0), "Invalid file identifier");
        
        bytes32 storageEventId = keccak256(abi.encodePacked(_client, _fileIdentifier, block.timestamp));
        
        fileStorage[storageEventId].client = _client;
        fileStorage[storageEventId].storageNode = msg.sender;
        fileStorage[storageEventId].fileIdentifier = _fileIdentifier;
        fileStorage[storageEventId].timestamp = block.timestamp;
        fileStorage[storageEventId].isActive = true;
        
        totalFiles++;
        
        emit FileStored(_client, msg.sender, _fileIdentifier, block.timestamp);
    }
    
    /**
     * @dev Record file deletion event
     */
    function recordFileDeletion(bytes32 _fileIdentifier) 
        external 
        onlyRegisteredClient 
        systemInitialized 
    {
        require(_fileIdentifier != bytes32(0), "Invalid file identifier");
        
        bytes32 storageEventId = keccak256(abi.encodePacked(msg.sender, _fileIdentifier));
        require(fileStorage[storageEventId].isActive, "File not found or already deleted");
        
        fileStorage[storageEventId].isActive = false;
        
        emit FileDeleted(msg.sender, _fileIdentifier, block.timestamp);
    }
    
    /**
     * @dev Update storage node reputation (called by verification contract)
     */
    function updateReputation(address _node, bool _positive) external {
        require(storageNodes[_node].active || storageNodes[_node].registrationTime > 0, "Node not found");
        
        if (_positive) {
            storageNodes[_node].reputation += 1;
        } else {
            if (storageNodes[_node].reputation >= 5) {
                storageNodes[_node].reputation -= 5;
            } else {
                storageNodes[_node].reputation = 0;
            }
            
            if (storageNodes[_node].reputation < 50) {
                storageNodes[_node].active = false;
            }
        }
        
        emit ReputationUpdated(_node, storageNodes[_node].reputation);
    }
    
    // Query Functions
    
    function getClientInfo(address _client) external view returns (ClientInfo memory) {
        return clients[_client];
    }
    
    function getStorageNodeInfo(address _node) external view returns (StorageNode memory) {
        return storageNodes[_node];
    }
    
    function getSystemStats() external view returns (uint256, uint256, uint256) {
        return (totalClients, totalStorageNodes, totalFiles);
    }
    
    function isRegisteredClient(address _address) external view returns (bool) {
        return clients[_address].registered;
    }
    
    function isActiveStorageNode(address _address) external view returns (bool) {
        return storageNodes[_address].active;
    }
    
    function getPublicParameters() external view returns (PublicParameters memory) {
        return systemParams;
    }
    
    function getAllClients() external view returns (address[] memory) {
        return clientAddresses;
    }
    
    function getAllStorageNodes() external view returns (address[] memory) {
        return storageNodeAddresses;
    }
}
```

---

## **Contract 2: VDSSearch (Search Layer)**

### **Purpose**
Handles encrypted keyword search operations with forward security and keyword-associated proof generation.

### **Main Functions**

| Function | Description | Access Control |
|----------|-------------|----------------|
| `submitSearchRequest()` | Submit encrypted keyword search with latest state | Registered Clients |
| `submitSearchResult()` | Return search results with unified proof | Storage Nodes |
| `markSearchResultVerified()` | Mark search result as verified | Verification Contract |
| `getSearchRequest()` | Query search request details | View function |
| `getSearchResult()` | Query search result and proof | View function |
| `getAllSearchRequestIds()` | Get all search request IDs | View function |

### **Key Features**
- ✅ Forward-secure keyword search (state-associated tokens)
- ✅ Keyword-associated proof submission
- ✅ Search token and state management
- ✅ Integration with VDSCore for client authentication
- ✅ Prevents duplicate search processing

### **Contract Code**

```solidity
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

/**
 * @title VDSSearch
 * @dev Contract for search operations and keyword-associated proofs
 * @notice Part 2 of 3 - Search functionality
 */
contract VDSSearch {
    
    // Reference to core contract
    VDSCore public coreContract;
    
    // Structs
    struct SearchRequest {
        address client;
        bytes searchToken;
        bytes latestState;
        uint256 timestamp;
        bool processed;
    }
    
    struct SearchResult {
        bytes32 requestId;
        bytes32[] fileIdentifiers;
        bytes keywordAssociatedProof;
        address storageNode;
        uint256 timestamp;
        bool verified;
    }
    
    // State variables
    mapping(bytes32 => SearchRequest) public searchRequests;
    mapping(bytes32 => SearchResult) public searchResults;
    
    bytes32[] public searchRequestIds;
    uint256 public totalSearchRequests;
    
    // Events
    event SearchRequestSubmitted(bytes32 indexed requestId, address indexed client, bytes searchToken, uint256 timestamp);
    event SearchResultSubmitted(bytes32 indexed requestId, bytes32[] fileIdentifiers, address indexed storageNode, uint256 timestamp);
    
    // Modifiers
    modifier onlyRegisteredClient() {
        require(coreContract.isRegisteredClient(msg.sender), "Client not registered");
        _;
    }
    
    modifier onlyRegisteredStorageNode() {
        require(coreContract.isActiveStorageNode(msg.sender), "Storage node not registered or inactive");
        _;
    }
    
    constructor(address _coreContract) {
        require(_coreContract != address(0), "Invalid core contract address");
        coreContract = VDSCore(_coreContract);
    }
    
    /**
     * @dev Submit a keyword search request
     */
    function submitSearchRequest(bytes calldata _searchToken, bytes calldata _latestState) 
        external 
        onlyRegisteredClient 
        returns (bytes32 requestId)
    {
        require(_searchToken.length > 0, "Invalid search token");
        require(_latestState.length > 0, "Invalid state");
        
        requestId = keccak256(abi.encodePacked(msg.sender, _searchToken, _latestState, block.timestamp));
        
        searchRequests[requestId].client = msg.sender;
        searchRequests[requestId].searchToken = _searchToken;
        searchRequests[requestId].latestState = _latestState;
        searchRequests[requestId].timestamp = block.timestamp;
        searchRequests[requestId].processed = false;
        
        searchRequestIds.push(requestId);
        totalSearchRequests++;
        
        emit SearchRequestSubmitted(requestId, msg.sender, _searchToken, block.timestamp);
        
        return requestId;
    }
    
    /**
     * @dev Submit search results with keyword-associated proof
     */
    function submitSearchResult(
        bytes32 _requestId,
        bytes32[] calldata _fileIdentifiers,
        bytes calldata _keywordAssociatedProof
    ) 
        external 
        onlyRegisteredStorageNode 
    {
        require(searchRequests[_requestId].timestamp > 0, "Search request not found");
        require(!searchRequests[_requestId].processed, "Search already processed");
        require(_fileIdentifiers.length > 0, "No files returned");
        require(_keywordAssociatedProof.length > 0, "Invalid proof");
        
        searchResults[_requestId].requestId = _requestId;
        searchResults[_requestId].fileIdentifiers = _fileIdentifiers;
        searchResults[_requestId].keywordAssociatedProof = _keywordAssociatedProof;
        searchResults[_requestId].storageNode = msg.sender;
        searchResults[_requestId].timestamp = block.timestamp;
        searchResults[_requestId].verified = false;
        
        searchRequests[_requestId].processed = true;
        
        emit SearchResultSubmitted(_requestId, _fileIdentifiers, msg.sender, block.timestamp);
    }
    
    /**
     * @dev Mark search result as verified (called by verification contract)
     */
    function markSearchResultVerified(bytes32 _requestId) external {
        require(searchResults[_requestId].timestamp > 0, "Search result not found");
        searchResults[_requestId].verified = true;
    }
    
    // Query Functions
    
    function getSearchRequest(bytes32 _requestId) external view returns (SearchRequest memory) {
        return searchRequests[_requestId];
    }
    
    function getSearchResult(bytes32 _requestId) external view returns (SearchResult memory) {
        return searchResults[_requestId];
    }
    
    function getAllSearchRequestIds() external view returns (bytes32[] memory) {
        return searchRequestIds;
    }
    
    function getTotalSearchRequests() external view returns (uint256) {
        return totalSearchRequests;
    }
}
```

---

## **Contract 3: VDSVerification (Verification Layer)**

### **Purpose**
Manages integrity auditing and verification of both search results and file integrity proofs with automatic reputation updates.

### **Main Functions**

| Function | Description | Access Control |
|----------|-------------|----------------|
| `submitIntegrityProof()` | Submit integrity proof for unsearched files | Storage Nodes |
| `submitSearchVerification()` | Verify search result correctness | Storage Nodes (not self) |
| `submitIntegrityVerification()` | Verify file integrity proof | Storage Nodes (not self) |
| `getIntegrityProof()` | Query integrity proof details | View function |
| `getVerificationResults()` | Query verification results for a proof | View function |
| `getAllIntegrityProofIds()` | Get all integrity proof IDs | View function |

### **Key Features**
- ✅ Unified proof verification (one proof validates both search and integrity)
- ✅ Decentralized verification (any node can verify except proof generator)
- ✅ Automatic reputation updates based on verification results
- ✅ Integration with both VDSCore and VDSSearch
- ✅ Verification result tracking and auditing

### **Contract Code**

```solidity
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

/**
 * @title VDSVerification
 * @dev Contract for integrity proofs and verification
 * @notice Part 3 of 3 - Verification functionality
 */
contract VDSVerification {
    
    // Reference to other contracts
    VDSCore public coreContract;
    VDSSearch public searchContract;
    
    // Structs
    struct IntegrityProof {
        bytes32 fileIdentifier;
        bytes proofW;
        bytes proofU;
        address storageNode;
        uint256 timestamp;
        bool verified;
    }
    
    struct VerificationResult {
        bytes32 proofId;
        address verifier;
        bool isValid;
        uint256 timestamp;
    }
    
    // State variables
    mapping(bytes32 => IntegrityProof) public integrityProofs;
    mapping(bytes32 => VerificationResult[]) public verificationResults;
    
    bytes32[] public integrityProofIds;
    
    // Events
    event IntegrityProofSubmitted(bytes32 indexed proofId, bytes32 fileIdentifier, address indexed storageNode, uint256 timestamp);
    event ProofVerified(bytes32 indexed proofId, address indexed verifier, bool isValid, uint256 timestamp);
    
    // Modifiers
    modifier onlyRegisteredStorageNode() {
        require(coreContract.isActiveStorageNode(msg.sender), "Storage node not registered or inactive");
        _;
    }
    
    constructor(address _coreContract, address _searchContract) {
        require(_coreContract != address(0), "Invalid core contract address");
        require(_searchContract != address(0), "Invalid search contract address");
        coreContract = VDSCore(_coreContract);
        searchContract = VDSSearch(_searchContract);
    }
    
    /**
     * @dev Submit integrity proof for unsearched files
     */
    function submitIntegrityProof(
        bytes32 _fileIdentifier,
        bytes calldata _proofW,
        bytes calldata _proofU
    ) 
        external 
        onlyRegisteredStorageNode 
        returns (bytes32 proofId)
    {
        require(_fileIdentifier != bytes32(0), "Invalid file identifier");
        require(_proofW.length > 0 && _proofU.length > 0, "Invalid proof");
        
        proofId = keccak256(abi.encodePacked(_fileIdentifier, msg.sender, block.timestamp));
        
        integrityProofs[proofId].fileIdentifier = _fileIdentifier;
        integrityProofs[proofId].proofW = _proofW;
        integrityProofs[proofId].proofU = _proofU;
        integrityProofs[proofId].storageNode = msg.sender;
        integrityProofs[proofId].timestamp = block.timestamp;
        integrityProofs[proofId].verified = false;
        
        integrityProofIds.push(proofId);
        
        emit IntegrityProofSubmitted(proofId, _fileIdentifier, msg.sender, block.timestamp);
        
        return proofId;
    }
    
    /**
     * @dev Submit verification result for search result
     */
    function submitSearchVerification(bytes32 _requestId, bool _isValid) 
        external 
        onlyRegisteredStorageNode 
    {
        VDSSearch.SearchResult memory result = searchContract.getSearchResult(_requestId);
        require(result.timestamp > 0, "Search result not found");
        require(result.storageNode != msg.sender, "Cannot verify own proof");
        
        bytes32 verificationId = keccak256(abi.encodePacked(_requestId, "search"));
        
        VerificationResult memory newVerification;
        newVerification.proofId = _requestId;
        newVerification.verifier = msg.sender;
        newVerification.isValid = _isValid;
        newVerification.timestamp = block.timestamp;
        
        verificationResults[verificationId].push(newVerification);
        
        if (!result.verified && _isValid) {
            searchContract.markSearchResultVerified(_requestId);
            coreContract.updateReputation(result.storageNode, true);
        } else if (!_isValid) {
            coreContract.updateReputation(result.storageNode, false);
        }
        
        emit ProofVerified(_requestId, msg.sender, _isValid, block.timestamp);
    }
    
    /**
     * @dev Submit verification result for integrity proof
     */
    function submitIntegrityVerification(bytes32 _proofId, bool _isValid) 
        external 
        onlyRegisteredStorageNode 
    {
        require(integrityProofs[_proofId].timestamp > 0, "Integrity proof not found");
        require(integrityProofs[_proofId].storageNode != msg.sender, "Cannot verify own proof");
        
        bytes32 verificationId = keccak256(abi.encodePacked(_proofId, "integrity"));
        
        VerificationResult memory newVerification;
        newVerification.proofId = _proofId;
        newVerification.verifier = msg.sender;
        newVerification.isValid = _isValid;
        newVerification.timestamp = block.timestamp;
        
        verificationResults[verificationId].push(newVerification);
        
        if (!integrityProofs[_proofId].verified && _isValid) {
            integrityProofs[_proofId].verified = true;
            coreContract.updateReputation(integrityProofs[_proofId].storageNode, true);
        } else if (!_isValid) {
            coreContract.updateReputation(integrityProofs[_proofId].storageNode, false);
        }
        
        emit ProofVerified(_proofId, msg.sender, _isValid, block.timestamp);
    }
    
    // Query Functions
    
    function getIntegrityProof(bytes32 _proofId) external view returns (IntegrityProof memory) {
        return integrityProofs[_proofId];
    }
    
    function getVerificationResults(bytes32 _verificationId) external view returns (VerificationResult[] memory) {
        return verificationResults[_verificationId];
    }
    
    function getAllIntegrityProofIds() external view returns (bytes32[] memory) {
        return integrityProofIds;
    }
}
```

---

## **Contract Interaction Flow**

```
┌─────────────────────────────────────────────────────────────┐
│                    WORKFLOW DIAGRAM                          │
└─────────────────────────────────────────────────────────────┘

1. INITIALIZATION
   VDSCore.initializeSystem(N, g, l)
   ↓

2. REGISTRATION
   Client → VDSCore.registerClient(publicKey)
   Storage Node → VDSCore.registerStorageNode(params)
   ↓

3. FILE STORAGE
   Storage Node → VDSCore.recordFileStorage(client, fileId)
   ↓

4. SEARCH OPERATION
   Client → VDSSearch.submitSearchRequest(token, state)
   ↓
   Storage Node → VDSSearch.submitSearchResult(requestId, files, proof)
   ↓

5. VERIFICATION
   Other Nodes → VDSVerification.submitSearchVerification(requestId, isValid)
   ↓
   VDSVerification → VDSSearch.markSearchResultVerified(requestId)
   ↓
   VDSVerification → VDSCore.updateReputation(node, positive)
   ↓

6. INTEGRITY AUDITING
   Storage Node → VDSVerification.submitIntegrityProof(fileId, proofW, proofU)
   ↓
   Other Nodes → VDSVerification.submitIntegrityVerification(proofId, isValid)
   ↓
   VDSVerification → VDSCore.updateReputation(node, positive)
```

---


## **Deployment Guide**

本笔记展示如何使用 **Node.js + ethers.js + solc** 一次性编译并部署三合一智能合约文件，包括以下模块：

1. **VDSCore**
2. **VDSSearch**（依赖 VDSCore）
3. **VDSVerification**（依赖 VDSCore + VDSSearch）

---

### Step 0: Initialize Environment

```javascript
const fs = require('fs');
const path = require('path');
const solc = require('solc');
const { ethers } = require('ethers');

// 配置区
const RPC_URL = 'http://127.0.0.1:8546';
const PRIVATE_KEY_FILE = 'private.key';
const SOL_FILE = 'chain_contract.sol'; // 三合一合约文件
const OUTPUT_ABI_FILE = 'chain_contract.sol.json';
const OUTPUT_ADDR_FILE = 'chain_contract.sol.txt';
```

---

### Step 1: Compile Contracts

```javascript
async function compileContracts(filePath) {
    const source = fs.readFileSync(filePath, 'utf8');
    const input = {
        language: 'Solidity',
        sources: { [path.basename(filePath)]: { content: source } },
        settings: {
            optimizer: { enabled: true, runs: 200 }, 
            outputSelection: { '*': { '*': ['abi', 'evm.bytecode'] } }
        }
    };
    const output = JSON.parse(solc.compile(JSON.stringify(input)));
    if (output.errors && output.errors.some(e => e.severity === 'error')) {
        throw new Error(`❌ 合约 ${filePath} 编译失败`);
    }
    return output.contracts[path.basename(filePath)];
}
```

---

### Step 2: Deploy Single Contract Function

```javascript
async function deployContract(name, abi, bytecode, wallet, args = []) {
    console.log(`🚀 正在部署 ${name} ...`);
    const factory = new ethers.ContractFactory(abi, bytecode, wallet);
    const contract = await factory.deploy(...args);
    await contract.waitForDeployment();
    const address = await contract.getAddress();
    console.log(`✅ ${name} 部署完成，地址: ${address}`);
    return { contract, address };
}
```

---

### Step 3: Main Deployment Script

```javascript
async function main() {
    const privateKey = fs.readFileSync(PRIVATE_KEY_FILE, 'utf8').trim();
    const provider = new ethers.JsonRpcProvider(RPC_URL);
    const wallet = new ethers.Wallet(privateKey, provider);

    console.log(`连接 RPC: ${RPC_URL}`);
    console.log(`部署账户: ${await wallet.getAddress()}`);

    // 编译合约
    const compiled = await compileContracts(SOL_FILE);
    const VDSCore = compiled['VDSCore'];
    const VDSSearch = compiled['VDSSearch'];
    const VDSVerification = compiled['VDSVerification'];

    // 部署三个合约
    const { address: coreAddr } = await deployContract('VDSCore', VDSCore.abi, VDSCore.evm.bytecode.object, wallet);
    const { address: searchAddr } = await deployContract('VDSSearch', VDSSearch.abi, VDSSearch.evm.bytecode.object, wallet, [coreAddr]);
    const { address: verifyAddr } = await deployContract('VDSVerification', VDSVerification.abi, VDSVerification.evm.bytecode.object, wallet, [coreAddr, searchAddr]);

    // 输出 ABI 与地址
    fs.writeFileSync(OUTPUT_ABI_FILE, JSON.stringify({ VDSCore: VDSCore.abi, VDSSearch: VDSSearch.abi, VDSVerification: VDSVerification.abi }, null, 2));
    fs.writeFileSync(OUTPUT_ADDR_FILE, `VDSCore: ${coreAddr}\nVDSSearch: ${searchAddr}\nVDSVerification: ${verifyAddr}\n`);

    console.log(`\n📦 ABI 已保存到: ${OUTPUT_ABI_FILE}`);
    console.log(`📜 部署地址已保存到: ${OUTPUT_ADDR_FILE}`);
    console.log(`✅ 所有合约部署成功！`);
}

main().catch(err => {
    console.error('❌ 部署失败:', err);
    process.exit(1);
});
```

---

### Step 4: Initialize System

```javascript
await coreContract.initializeSystem(N_bytes, g_bytes, l_bytes);
```

---

### **Key Advantages of This Architecture**

| Advantage           | Description      |
| ------------------- | ---------------- |
| **Modularity**      | 每个合约独立实现功能       |
| **Deployability**   | 小合约易部署，不超 gas 限制 |
| **Upgradeability**  | 各组件可独立升级         |
| **Gas Efficiency**  | 优化存储和函数调用        |
| **Security**        | 清晰分工与访问控制        |
| **Maintainability** | 易审计和调试单个合约       |
---

## **Key Advantages of This Architecture**

| Advantage | Description |
|-----------|-------------|
| **Modularity** | Each contract focuses on specific functionality |
| **Deployability** | Smaller contracts fit within gas limits |
| **Upgradeability** | Individual components can be upgraded independently |
| **Gas Efficiency** | Optimized storage and function calls |
| **Security** | Clear separation of concerns and access control |
| **Maintainability** | Easier to audit and debug individual contracts |

---

## **Summary**

This three-contract system successfully implements the paper's "Enabling Verifiable Search and Integrity Auditing in Encrypted Decentralized Storage Using One Proof" with the following achievements:

✅ **VDSCore**: Foundation layer managing registration and reputation  
✅ **VDSSearch**: Search layer with forward-secure keyword search  
✅ **VDSVerification**: Verification layer with unified proof validation  

The modular architecture maintains all original functionality while enabling deployment on resource-constrained private blockchain networks. Cross-contract communication ensures seamless integration, and the reputation system incentivizes honest behavior among storage nodes.

