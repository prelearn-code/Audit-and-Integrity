// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

/**
 * @title DecentralizedStorage
 * @dev Smart contract for verifiable search and integrity auditing in encrypted decentralized storage
 */
contract DecentralizedStorage {
    
    // ==================== State Variables ====================
    
    bytes public PP_DIGEST;
    bytes32 public DOMAIN;
    uint256 public epoch;
    bool public initialized;
    address public nodeAdmin;
    
    uint256 public totalClients;
    uint256 public totalStorages;
    uint256 public requestCounter;
    
    // ==================== Structures ====================
    
    struct ClientInfo {
        bool registered;
        bytes pk;
        string metadata;
        uint256 joinedAt;
    }
    
    struct StorageInfo {
        bool registered;
        address storageSigner;
        string endpoint;
        string region;
        uint256 capacity;
        string metadata;
        uint256 joinedAt;
        uint256 lastSeen;
    }
    
    struct SearchRequest {
        address client;
        bytes T;
        bytes32 std;
        uint256 timestamp;
    }
    
    struct SearchProof {
        bytes32 storageId;
        bytes32[] resultIDs;
        bytes[] wList;
        bytes[] uList;
        bytes signature;
        uint256 timestamp;
    }
    
    struct AuditProof {
        bytes32 fileId;
        bytes32 storageId;
        bytes w;
        bytes u;
        uint256 period;
        bytes signature;
        uint256 timestamp;
    }
    
    struct VerifyReceipt {
        bytes32 referenceId;
        bytes32 storageId;
        bool isValid;
        bytes signature;
        uint256 timestamp;
    }
    
    // ==================== Mappings ====================
    
    mapping(address => ClientInfo) public clients;
    mapping(bytes32 => StorageInfo) public storages;
    mapping(bytes32 => SearchRequest) public searchRequests;
    mapping(bytes32 => SearchProof) public searchProofs;
    mapping(bytes32 => AuditProof[]) public auditProofs;
    mapping(bytes32 => VerifyReceipt[]) public verifyReceipts;
    
    // ==================== Events ====================
    
    event GlobalParamsInitialized(bytes ppDigest, bytes32 domain, uint256 epoch);
    event ClientRegistered(address indexed client, bytes pk, uint256 timestamp);
    event ClientUnregistered(address indexed client, uint256 timestamp);
    event StorageRegistered(bytes32 indexed storageId, address signer, string endpoint, uint256 timestamp);
    event StorageUnregistered(bytes32 indexed storageId, uint256 timestamp);
    event SearchPublished(bytes32 indexed reqId, address indexed client, bytes T, bytes32 std, uint256 timestamp);
    event SearchProofSubmitted(bytes32 indexed reqId, bytes32 indexed storageId, uint256 resultCount, uint256 timestamp);
    event AuditProofSubmitted(bytes32 indexed fileId, bytes32 indexed storageId, uint256 period, uint256 timestamp);
    event VerifyReceiptSubmitted(bytes32 indexed referenceId, bytes32 indexed storageId, bool isValid, uint256 timestamp);
    
    // ==================== Modifiers ====================
    
    modifier onlyNodeAdmin() {
        require(msg.sender == nodeAdmin, "Only node admin can call this function");
        _;
    }
    
    modifier onlyRegisteredClient() {
        require(clients[msg.sender].registered, "Client not registered");
        _;
    }
    
    modifier notInitialized() {
        require(!initialized, "Contract already initialized");
        _;
    }
    
    modifier onlyInitialized() {
        require(initialized, "Contract not initialized");
        _;
    }
    
    // ==================== Constructor ====================
    
    constructor(address _nodeAdmin) {
        require(_nodeAdmin != address(0), "Invalid node admin address");
        nodeAdmin = _nodeAdmin;
        epoch = 1;
        initialized = false;
    }
    
    // ==================== Global Initialization ====================
    
    function initGlobalParams(bytes calldata ppDigest, bytes32 domain) 
        external 
        onlyNodeAdmin 
        notInitialized 
    {
        require(ppDigest.length > 0, "PP digest cannot be empty");
        require(domain != bytes32(0), "Domain cannot be zero");
        
        PP_DIGEST = ppDigest;
        DOMAIN = domain;
        initialized = true;
        
        emit GlobalParamsInitialized(ppDigest, domain, epoch);
    }
    
    function getGlobalParams() 
        external 
        view 
        returns (bytes memory ppDigest, bytes32 domain, uint256 currentEpoch) 
    {
        return (PP_DIGEST, DOMAIN, epoch);
    }
    
    // ==================== Client Management ====================
    
    function registerClient(bytes calldata pk, string calldata metadata) 
        external 
        onlyInitialized 
    {
        require(!clients[msg.sender].registered, "Client already registered");
        require(pk.length > 0, "Public key cannot be empty");
        
        clients[msg.sender] = ClientInfo({
            registered: true,
            pk: pk,
            metadata: metadata,
            joinedAt: block.timestamp
        });
        
        totalClients++;
        
        emit ClientRegistered(msg.sender, pk, block.timestamp);
    }
    
    function unregisterClient() 
        external 
        onlyRegisteredClient 
    {
        delete clients[msg.sender];
        totalClients--;
        
        emit ClientUnregistered(msg.sender, block.timestamp);
    }
    
    function getClientInfo(address clientAddr) 
        external 
        view 
        returns (ClientInfo memory) 
    {
        return clients[clientAddr];
    }
    
    // ==================== Storage Node Management ====================
    
    function registerStorage(
        bytes32 storageId,
        address signer,
        string calldata endpoint,
        string calldata region,
        uint256 capacity,
        string calldata metadata
    ) 
        external 
        onlyNodeAdmin 
        onlyInitialized 
    {
        require(storageId != bytes32(0), "Storage ID cannot be zero");
        require(signer != address(0), "Signer address cannot be zero");
        require(!storages[storageId].registered, "Storage already registered");
        require(bytes(endpoint).length > 0, "Endpoint cannot be empty");
        
        storages[storageId] = StorageInfo({
            registered: true,
            storageSigner: signer,
            endpoint: endpoint,
            region: region,
            capacity: capacity,
            metadata: metadata,
            joinedAt: block.timestamp,
            lastSeen: block.timestamp
        });
        
        totalStorages++;
        
        emit StorageRegistered(storageId, signer, endpoint, block.timestamp);
    }
    
    function unregisterStorage(bytes32 storageId) 
        external 
        onlyNodeAdmin 
    {
        require(storages[storageId].registered, "Storage not registered");
        
        delete storages[storageId];
        totalStorages--;
        
        emit StorageUnregistered(storageId, block.timestamp);
    }
    
    function updateStorageLastSeen(bytes32 storageId) 
        external 
        onlyNodeAdmin 
    {
        require(storages[storageId].registered, "Storage not registered");
        storages[storageId].lastSeen = block.timestamp;
    }
    
    function getStorageInfo(bytes32 storageId) 
        external 
        view 
        returns (StorageInfo memory) 
    {
        return storages[storageId];
    }
    
    // ==================== Verifiable Search ====================
    
    function publishSearch(bytes calldata T, bytes32 std) 
        external 
        onlyRegisteredClient 
        returns (bytes32 reqId) 
    {
        require(T.length > 0, "Search token cannot be empty");
        require(std != bytes32(0), "State cannot be zero");
        
        requestCounter++;
        reqId = keccak256(abi.encodePacked(msg.sender, T, std, block.timestamp, requestCounter));
        
        searchRequests[reqId] = SearchRequest({
            client: msg.sender,
            T: T,
            std: std,
            timestamp: block.timestamp
        });
        
        emit SearchPublished(reqId, msg.sender, T, std, block.timestamp);
        
        return reqId;
    }
    
    function submitSearchProof(
        bytes32 reqId,
        bytes32 storageId,
        bytes32[] calldata resultIDs,
        bytes[] calldata wList,
        bytes[] calldata uList,
        bytes calldata signature
    ) 
        external 
        onlyNodeAdmin 
    {
        require(searchRequests[reqId].client != address(0), "Search request not found");
        require(storages[storageId].registered, "Storage not registered");
        require(resultIDs.length == wList.length, "Result IDs and w list length mismatch");
        require(resultIDs.length == uList.length, "Result IDs and u list length mismatch");
        
        // Verify signature
        bytes32 messageHash = keccak256(abi.encode(
            reqId,
            storageId,
            resultIDs,  // 允许动态数组
            wList,      // 允许动态数组
            uList,      // 允许动态数组
            block.chainid
            )
        );
        bytes32 ethSignedMessageHash = getEthSignedMessageHash(messageHash);
        address recoveredSigner = recoverSigner(ethSignedMessageHash, signature);
        
        require(recoveredSigner == storages[storageId].storageSigner, "Invalid signature");
        
        searchProofs[reqId] = SearchProof({
            storageId: storageId,
            resultIDs: resultIDs,
            wList: wList,
            uList: uList,
            signature: signature,
            timestamp: block.timestamp
        });
        
        storages[storageId].lastSeen = block.timestamp;
        
        emit SearchProofSubmitted(reqId, storageId, resultIDs.length, block.timestamp);
    }
    
    function getSearchRequest(bytes32 reqId) 
        external 
        view 
        returns (SearchRequest memory) 
    {
        return searchRequests[reqId];
    }
    
    function getSearchProof(bytes32 reqId) 
        external 
        view 
        returns (SearchProof memory) 
    {
        return searchProofs[reqId];
    }
    
    // ==================== Integrity Auditing ====================
    
    function submitAuditProof(
        bytes32 fileId,
        bytes32 storageId,
        bytes calldata w,
        bytes calldata u,
        uint256 period,
        bytes calldata signature
    ) 
        external 
        onlyNodeAdmin 
    {
        require(fileId != bytes32(0), "File ID cannot be zero");
        require(storages[storageId].registered, "Storage not registered");
        require(w.length > 0 && u.length > 0, "Proof components cannot be empty");
        
        // Verify signature
        bytes32 messageHash = keccak256(abi.encodePacked(
            fileId, 
            storageId, 
            w, 
            u, 
            period,
            block.chainid
        ));
        bytes32 ethSignedMessageHash = getEthSignedMessageHash(messageHash);
        address recoveredSigner = recoverSigner(ethSignedMessageHash, signature);
        
        require(recoveredSigner == storages[storageId].storageSigner, "Invalid signature");
        
        auditProofs[fileId].push(AuditProof({
            fileId: fileId,
            storageId: storageId,
            w: w,
            u: u,
            period: period,
            signature: signature,
            timestamp: block.timestamp
        }));
        
        storages[storageId].lastSeen = block.timestamp;
        
        emit AuditProofSubmitted(fileId, storageId, period, block.timestamp);
    }
    
    function getAuditProofs(bytes32 fileId) 
        external 
        view 
        returns (AuditProof[] memory) 
    {
        return auditProofs[fileId];
    }
    
    function getLatestAuditProof(bytes32 fileId) 
        external 
        view 
        returns (AuditProof memory) 
    {
        require(auditProofs[fileId].length > 0, "No audit proofs found");
        return auditProofs[fileId][auditProofs[fileId].length - 1];
    }
    
    // ==================== Verification Receipts ====================
    
    function submitVerifyReceipt(
        bytes32 referenceId,
        bytes32 storageId,
        bool isValid,
        bytes calldata signature
    ) 
        external 
        onlyNodeAdmin 
    {
        require(referenceId != bytes32(0), "Reference ID cannot be zero");
        require(storages[storageId].registered, "Storage not registered");
        
        // Verify signature
        bytes32 messageHash = keccak256(abi.encodePacked(
            referenceId, 
            storageId, 
            isValid,
            block.chainid
        ));
        bytes32 ethSignedMessageHash = getEthSignedMessageHash(messageHash);
        address recoveredSigner = recoverSigner(ethSignedMessageHash, signature);
        
        require(recoveredSigner == storages[storageId].storageSigner, "Invalid signature");
        
        verifyReceipts[referenceId].push(VerifyReceipt({
            referenceId: referenceId,
            storageId: storageId,
            isValid: isValid,
            signature: signature,
            timestamp: block.timestamp
        }));
        
        storages[storageId].lastSeen = block.timestamp;
        
        emit VerifyReceiptSubmitted(referenceId, storageId, isValid, block.timestamp);
    }
    
    function getVerifyReceipts(bytes32 referenceId) 
        external 
        view 
        returns (VerifyReceipt[] memory) 
    {
        return verifyReceipts[referenceId];
    }
    
    function getVerificationConsensus(bytes32 referenceId) 
        external 
        view 
        returns (uint256 validCount, uint256 invalidCount, uint256 totalCount) 
    {
        VerifyReceipt[] memory receipts = verifyReceipts[referenceId];
        totalCount = receipts.length;
        
        for (uint256 i = 0; i < totalCount; i++) {
            if (receipts[i].isValid) {
                validCount++;
            } else {
                invalidCount++;
            }
        }
        
        return (validCount, invalidCount, totalCount);
    }
    
    // ==================== Signature Verification ====================
    
    function getEthSignedMessageHash(bytes32 messageHash) 
        internal 
        pure 
        returns (bytes32) 
    {
        return keccak256(abi.encodePacked("\x19Ethereum Signed Message:\n32", messageHash));
    }
    
    function recoverSigner(bytes32 ethSignedMessageHash, bytes memory signature) 
        internal 
        pure 
        returns (address) 
    {
        require(signature.length == 65, "Invalid signature length");
        
        bytes32 r;
        bytes32 s;
        uint8 v;
        
        assembly {
            r := mload(add(signature, 32))
            s := mload(add(signature, 64))
            v := byte(0, mload(add(signature, 96)))
        }
        
        return ecrecover(ethSignedMessageHash, v, r, s);
    }
    
    // ==================== Admin Functions ====================
    
    function updateNodeAdmin(address newAdmin) 
        external 
        onlyNodeAdmin 
    {
        require(newAdmin != address(0), "Invalid admin address");
        nodeAdmin = newAdmin;
    }
    
    function getStatistics() 
        external 
        view 
        returns (uint256, uint256, uint256) 
    {
        return (totalClients, totalStorages, requestCounter);
    }
}
