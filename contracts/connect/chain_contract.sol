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

    // Insert information about stored files 
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
    // Insert informations to the blockchain logs.
    event SystemInitialized(bytes N, bytes g, bytes l, uint256 timestamp);
    event ClientRegistered(address indexed client, bytes publicKey, uint256 timestamp);
    event StorageNodeRegistered(address indexed node, uint256 timestamp);
    event FileStored(address indexed client, address indexed storageNode, bytes32 fileIdentifier, uint256 timestamp);
    event FileDeleted(address indexed client, bytes32 fileIdentifier, uint256 timestamp);
    event ReputationUpdated(address indexed storageNode, uint256 newReputation);
    
    // Modifiers Initialization check
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

    // 存储提交的公共参数
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
        
        // 搜索操作的ID
        searchResults[_requestId].requestId = _requestId;
        // 搜搜到文件的ID列表
        searchResults[_requestId].fileIdentifiers = _fileIdentifiers;
        // 关键词关联证明
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