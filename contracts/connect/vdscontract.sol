// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

/**
 * @title VDSBlockchainCore
 * @dev Decentralized Storage Ledger Contract
 * @notice 基于 Song et al., 2025 论文的可验证分布式存储区块链合约
 * 职责：公证层（记录、协调、审计）- 不执行加密计算
 */
contract VDSBlockchainCore {
    
    // ========== 数据结构 ==========
    
    struct ClientInfo {
        string pubKey;
        uint256 joinedAt;
        bool isRegistered;
    }
    
    struct StorageNodeInfo {
        string nodeID;
        string endpoint;
        uint256 lastAudit;
        uint256 reputation; // 信誉分数
        bool isActive;
    }
    
    struct SearchRequest {
        bytes32 keywordHash;
        bytes32 tokenHash;
        bytes32 stateHash;
        address requester;
        uint256 timestamp;
        bool processed;
    }
    
    enum ProofType { SEARCH, AUDIT }
    
    struct ProofInfo {
        bytes32 fileID;
        address node;
        string proofURI; // IPFS/链下存储URI
        uint256 timestamp;
        ProofType proofType;
        bytes32[] results; // 搜索结果集（文件ID列表）
        bool verified;
        uint256 verificationCount;
    }
    
    struct VerificationInfo {
        address verifier;
        bool result;
        uint256 timestamp;
    }
    
    // ========== 状态变量 ==========
    
    mapping(address => ClientInfo) public clients;
    mapping(address => StorageNodeInfo) public storageNodes;
    mapping(bytes32 => SearchRequest) public searchRequests;
    mapping(bytes32 => ProofInfo) public proofs; // keywordHash/fileID -> Proof
    mapping(bytes32 => VerificationInfo[]) public verifications;
    
    address public admin;
    uint256 public minVerifications = 3; // 最小验证节点数
    uint256 public reputationThreshold = 100; // 信誉阈值
    
    // ========== 事件 ==========
    
    event ClientRegistered(address indexed client, uint256 timestamp);
    event NodeRegistered(address indexed node, string nodeID, uint256 timestamp);
    event SearchRequested(bytes32 indexed keywordHash, address indexed requester, uint256 timestamp);
    event ProofSubmitted(bytes32 indexed proofKey, address indexed node, ProofType proofType, uint256 timestamp);
    event ProofVerified(bytes32 indexed proofKey, address indexed verifier, bool result, uint256 timestamp);
    event NodeAudited(address indexed node, uint256 reputation, bool status, uint256 timestamp);
    
    // ========== 修饰器 ==========
    
    modifier onlyAdmin() {
        require(msg.sender == admin, "Only admin");
        _;
    }
    
    modifier onlyRegisteredClient() {
        require(clients[msg.sender].isRegistered, "Client not registered");
        _;
    }
    
    modifier onlyRegisteredNode() {
        require(storageNodes[msg.sender].isActive, "Node not active");
        _;
    }
    
    // ========== 构造函数 ==========
    
    constructor() {
        admin = msg.sender;
    }
    
    // ========== 1. 身份与注册 ==========
    
    /**
     * @notice 客户端注册（对应 KeyGen 阶段）
     * @param pubKey 客户端公钥
     */
    function registerClient(string memory pubKey) external {
        require(!clients[msg.sender].isRegistered, "Already registered");
        require(bytes(pubKey).length > 0, "Invalid pubKey");
        
        clients[msg.sender] = ClientInfo({
            pubKey: pubKey,
            joinedAt: block.timestamp,
            isRegistered: true
        });
        
        emit ClientRegistered(msg.sender, block.timestamp);
    }
    
    /**
     * @notice 存储节点注册（对应 Setup 阶段）
     * @param nodeID 节点标识符
     * @param endpoint 节点访问端点
     */
    function registerNode(string memory nodeID, string memory endpoint) external {
        require(!storageNodes[msg.sender].isActive, "Already registered");
        require(bytes(nodeID).length > 0 && bytes(endpoint).length > 0, "Invalid parameters");
        
        storageNodes[msg.sender] = StorageNodeInfo({
            nodeID: nodeID,
            endpoint: endpoint,
            lastAudit: block.timestamp,
            reputation: 100, // 初始信誉
            isActive: true
        });
        
        emit NodeRegistered(msg.sender, nodeID, block.timestamp);
    }
    
    // ========== 2. 搜索请求记录 ==========
    
    /**
     * @notice 客户端发起搜索请求（将 T, st 上链）
     * @param keywordHash 关键词哈希
     * @param tokenHash 搜索令牌哈希
     * @param stateHash 状态哈希
     */
    function requestSearch(
        bytes32 keywordHash,
        bytes32 tokenHash,
        bytes32 stateHash
    ) external onlyRegisteredClient {
        require(keywordHash != bytes32(0), "Invalid keywordHash");
        require(!searchRequests[keywordHash].processed, "Request already exists");
        
        searchRequests[keywordHash] = SearchRequest({
            keywordHash: keywordHash,
            tokenHash: tokenHash,
            stateHash: stateHash,
            requester: msg.sender,
            timestamp: block.timestamp,
            processed: false
        });
        
        emit SearchRequested(keywordHash, msg.sender, block.timestamp);
    }
    
    // ========== 3. 证明存证 ==========
    
    /**
     * @notice 存储节点提交搜索结果与证明（对应 Proof-Search）
     * @param keywordHash 关键词哈希
     * @param results 搜索结果集（文件ID数组）
     * @param proofURI 证明数据URI（IPFS/链下）
     */
    function submitSearchProof(
        bytes32 keywordHash,
        bytes32[] memory results,
        string memory proofURI
    ) external onlyRegisteredNode {
        require(searchRequests[keywordHash].timestamp > 0, "Search request not found");
        require(!proofs[keywordHash].verified, "Proof already submitted");
        require(bytes(proofURI).length > 0, "Invalid proofURI");
        
        proofs[keywordHash] = ProofInfo({
            fileID: keywordHash,
            node: msg.sender,
            proofURI: proofURI,
            timestamp: block.timestamp,
            proofType: ProofType.SEARCH,
            results: results,
            verified: false,
            verificationCount: 0
        });
        
        searchRequests[keywordHash].processed = true;
        
        emit ProofSubmitted(keywordHash, msg.sender, ProofType.SEARCH, block.timestamp);
    }
    
    /**
     * @notice 存储节点提交周期完整性证明（对应 Proof-Audit）
     * @param fileID 文件标识
     * @param proofURI 审计证明URI
     */
    function submitAuditProof(
        bytes32 fileID,
        string memory proofURI
    ) external onlyRegisteredNode {
        require(fileID != bytes32(0), "Invalid fileID");
        require(bytes(proofURI).length > 0, "Invalid proofURI");
        
        bytes32 auditKey = keccak256(abi.encodePacked(fileID, msg.sender, block.timestamp));
        
        proofs[auditKey] = ProofInfo({
            fileID: fileID,
            node: msg.sender,
            proofURI: proofURI,
            timestamp: block.timestamp,
            proofType: ProofType.AUDIT,
            results: new bytes32[](0),
            verified: false,
            verificationCount: 0
        });
        
        emit ProofSubmitted(auditKey, msg.sender, ProofType.AUDIT, block.timestamp);
    }
    
    // ========== 4. 验证结果登记 ==========
    
    /**
     * @notice 其他节点验证搜索证明（对应 Verify.Search）
     * @param keywordHash 关键词哈希
     * @param result 验证结果（true=通过, false=失败）
     */
    function verifySearchProof(
        bytes32 keywordHash,
        bool result
    ) external onlyRegisteredNode {
        require(proofs[keywordHash].timestamp > 0, "Proof not found");
        require(proofs[keywordHash].node != msg.sender, "Cannot verify own proof");
        
        verifications[keywordHash].push(VerificationInfo({
            verifier: msg.sender,
            result: result,
            timestamp: block.timestamp
        }));
        
        proofs[keywordHash].verificationCount++;
        
        // 达到最小验证数后标记为已验证
        if (proofs[keywordHash].verificationCount >= minVerifications) {
            proofs[keywordHash].verified = true;
            _updateNodeReputation(proofs[keywordHash].node, result);
        }
        
        emit ProofVerified(keywordHash, msg.sender, result, block.timestamp);
    }
    
    /**
     * @notice 其他节点验证审计证明（对应 Verify.Audit）
     * @param auditKey 审计证明键
     * @param result 验证结果
     */
    function verifyAuditProof(
        bytes32 auditKey,
        bool result
    ) external onlyRegisteredNode {
        require(proofs[auditKey].timestamp > 0, "Proof not found");
        require(proofs[auditKey].node != msg.sender, "Cannot verify own proof");
        require(proofs[auditKey].proofType == ProofType.AUDIT, "Not an audit proof");
        
        verifications[auditKey].push(VerificationInfo({
            verifier: msg.sender,
            result: result,
            timestamp: block.timestamp
        }));
        
        proofs[auditKey].verificationCount++;
        
        if (proofs[auditKey].verificationCount >= minVerifications) {
            proofs[auditKey].verified = true;
            _updateNodeReputation(proofs[auditKey].node, result);
        }
        
        emit ProofVerified(auditKey, msg.sender, result, block.timestamp);
    }
    
    // ========== 5. 审计追踪与激励 ==========
    
    /**
     * @notice 周期性审计节点（检查活跃性与信誉）
     * @param node 节点地址
     */
    function auditNode(address node) external onlyAdmin {
        require(storageNodes[node].isActive, "Node not active");
        
        StorageNodeInfo storage nodeInfo = storageNodes[node];
        nodeInfo.lastAudit = block.timestamp;
        
        // 信誉低于阈值则停用节点
        bool status = nodeInfo.reputation >= reputationThreshold;
        if (!status) {
            nodeInfo.isActive = false;
        }
        
        emit NodeAudited(node, nodeInfo.reputation, status, block.timestamp);
    }
    
    /**
     * @dev 更新节点信誉（内部函数）
     * @param node 节点地址
     * @param verifySuccess 验证是否成功
     */
    function _updateNodeReputation(address node, bool verifySuccess) internal {
        if (verifySuccess) {
            storageNodes[node].reputation += 10; // 奖励
        } else {
            if (storageNodes[node].reputation > 20) {
                storageNodes[node].reputation -= 20; // 惩罚
            } else {
                storageNodes[node].reputation = 0;
            }
        }
    }
    
    // ========== 查询函数 ==========
    
    /**
     * @notice 获取搜索请求详情
     */
    function getSearchRequest(bytes32 keywordHash) external view returns (SearchRequest memory) {
        return searchRequests[keywordHash];
    }
    
    /**
     * @notice 获取证明信息
     */
    function getProof(bytes32 proofKey) external view returns (ProofInfo memory) {
        return proofs[proofKey];
    }
    
    /**
     * @notice 获取验证记录
     */
    function getVerifications(bytes32 proofKey) external view returns (VerificationInfo[] memory) {
        return verifications[proofKey];
    }
    
    /**
     * @notice 获取节点信誉
     */
    function getNodeReputation(address node) external view returns (uint256) {
        return storageNodes[node].reputation;
    }
    
    // ========== 管理函数 ==========
    
    /**
     * @notice 设置最小验证节点数
     */
    function setMinVerifications(uint256 _minVerifications) external onlyAdmin {
        require(_minVerifications > 0, "Invalid value");
        minVerifications = _minVerifications;
    }
    
    /**
     * @notice 设置信誉阈值
     */
    function setReputationThreshold(uint256 _threshold) external onlyAdmin {
        reputationThreshold = _threshold;
    }
    
    /**
     * @notice 停用节点（管理员权限）
     */
    function deactivateNode(address node) external onlyAdmin {
        require(storageNodes[node].isActive, "Node already inactive");
        storageNodes[node].isActive = false;
    }
    
    /**
     * @notice 激活节点（管理员权限）
     */
    function activateNode(address node) external onlyAdmin {
        require(!storageNodes[node].isActive, "Node already active");
        require(bytes(storageNodes[node].nodeID).length > 0, "Node not registered");
        storageNodes[node].isActive = true;
    }
}
