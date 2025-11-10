#ifndef DECENTRALIZED_STORAGE_CLIENT_H
#define DECENTRALIZED_STORAGE_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <gmp.h>
#include <pbc/pbc.h>
#include <json/json.h>

/**
 * @brief 本地加密存储客户端 v4.0
 * 实现可验证的可搜索加密与前向安全性
 * 
 * v4.0 主要变更（方案A重构）：
 * ===================================
 * 【核心修改】
 * - initialize() 统一从 public_params.json 加载所有参数（N, g, μ）
 * - 删除 system_params.json 依赖，配对参数硬编码
 * - generateKeys() 简化为仅生成密钥，不再加载参数
 * - 确保客户端与 Storage Node 参数完全一致
 * 
 * 【修复的问题】
 * - 修复 μ 参数未正确加载导致的认证标签错误
 * - 修复 N 值在多个文件中不一致的问题
 * - 修复 g 被重复加载的问题
 * 
 * 【接口变更】
 * - initialize(public_params_file) - 新增参数文件路径
 * - generateKeys() - 移除参数，简化为仅生成密钥
 * - loadKeys() - 增加初始化状态检查
 */
class StorageClient {
public:
    /**
     * @brief 构造函数
     */
    StorageClient();
    
    /**
     * @brief 析构函数 - 清理密码学资源
     */
    ~StorageClient();
    
    /**
     * @brief 初始化客户端（v4.0重构）
     * @param public_params_file Storage Node生成的公共参数文件路径
     * @return 成功返回true
     * 
     * v4.0新逻辑：
     * 1. 初始化配对参数（硬编码Type A曲线）
     * 2. 从 public_params.json 加载：
     *    - N: RSA模数
     *    - g: 生成元（G₁群）
     *    - μ: 认证参数（G₁群）
     * 3. 验证参数有效性
     * 
     * 注意：必须先调用此函数，再调用其他功能
     */
    bool initialize(const std::string& public_params_file = "public_params.json");
    
    /**
     * @brief 生成客户端密钥（v4.0简化）
     * @return 成功返回true
     * 
     * v4.0新逻辑：
     * 1. 检查是否已初始化（必须先调用initialize）
     * 2. 生成随机密钥：mk, sk, ek
     * 3. 计算公钥：pk = g^sk
     * 4. 保存 private_key.dat（二进制）
     * 5. 保存 public_key.json（JSON格式，包含pk序列化）
     * 
     * 注意：不再从文件加载参数，仅生成密钥
     */
    bool generateKeys();
    
    /**
     * @brief 加密文件并生成元数据
     * @param file_path 输入文件路径
     * @param keywords 关键词列表
     * @param output_prefix 输出文件前缀
     * @param insert_json_path 生成的insert.json路径（供Storage Node使用）
     * @return 成功返回true
     * 
     * 输出文件：
     * 1. [prefix].enc - 加密文件
     * 2. insert.json - 供Storage Node的插入数据
     * 3. [filename]_metadata.json - 本地元数据
     */
    bool encryptFile(const std::string& file_path, 
                     const std::vector<std::string>& keywords,
                     const std::string& output_prefix,
                     const std::string& insert_json_path = "insert.json");
    
    /**
     * @brief 解密文件
     * @param encrypted_file 加密文件路径
     * @param output_path 输出文件路径
     * @return 成功返回true
     */
    bool decryptFile(const std::string& encrypted_file, 
                     const std::string& output_path);
    
    /**
     * @brief 获取公钥
     * @return 序列化的公钥
     */
    std::string getPublicKey();
    
    /**
     * @brief 保存密钥到文件
     * @param key_file 密钥文件路径
     * @return 成功返回true
     */
    bool saveKeys(const std::string& key_file);
    
    /**
     * @brief 从文件加载密钥（v4.0增强）
     * @param key_file 密钥文件路径
     * @return 成功返回true
     * 
     * v4.0新增检查：
     * - 如果尚未初始化，会提示用户先调用initialize()
     * - 防止在参数未加载时加载密钥导致的错误
     */
    bool loadKeys(const std::string& key_file);
    
    // ============ 关键词状态管理功能 ============
    
    /**
     * @brief 从文件加载关键词状态
     * @param file_path 状态文件路径
     * @return 成功返回true
     */
    bool loadKeywordStates(const std::string& file_path);
    
    /**
     * @brief 保存关键词状态到文件
     * @param file_path 输出文件路径
     * @return 成功返回true
     */
    bool saveKeywordStates(const std::string& file_path);
    
    /**
     * @brief 更新关键词状态（添加历史记录）
     * @param keyword 关键词
     * @param new_state 新状态值
     * @param file_id 关联的文件ID
     * @return 成功返回true
     */
    bool updateKeywordState(const std::string& keyword, 
                           const std::string& new_state,
                           const std::string& file_id);
    
    /**
     * @brief 查询关键词的当前状态
     * @param keyword 关键词
     * @return 格式化的状态信息字符串
     */
    std::string queryKeywordState(const std::string& keyword);

private:
    // ============ 密码学操作 ============
    
    bool encryptFileData(const std::vector<unsigned char>& plaintext,
                        std::vector<unsigned char>& ciphertext);
    
    bool decryptFileData(const std::vector<unsigned char>& ciphertext,
                        std::vector<unsigned char>& plaintext);
    
    /**
     * @brief 生成认证标签
     * @param file_id 文件ID
     * @param ciphertext 密文
     * @param auth_tags 输出的认证标签
     * @return 成功返回true
     * 
     * 算法：σ_i = [H_2(ID_F||i) * ∏_{j=1}^s μ^{c_{i,j}}]^sk
     */
    bool generateAuthTags(const std::string& file_id,
                         const std::vector<unsigned char>& ciphertext,
                         std::vector<std::string>& auth_tags);
    
    /**
     * @brief 生成状态关联令牌
     * @param file_id 文件ID
     * @param Ti 搜索令牌
     * @param current_state 当前状态
     * @param previous_state 前一状态（为空表示第一个文件）
     * @param kt_output 输出的关键词标签
     * @return 成功返回true
     * 
     * 算法：
     * - 有前一状态: kt^{w_i} = [H_2(ID_F) * H_2(st_d||Ti) / H_2(st_{d-1}||Ti)]^{sk}
     * - 无前一状态: kt^{w_i} = [H_2(ID_F) * H_2(st_d||Ti)]^{sk}
     */
    bool generateStateAssociatedToken(const std::string& file_id,
                                      const std::string& Ti,
                                      const std::string& current_state,
                                      const std::string& previous_state,
                                      std::string& kt_output);
    
    // ============ 哈希函数 ============
    
    /**
     * @brief H1: {0,1}* → Z_N（哈希到大整数）
     */
    void computeHashH1(const std::string& input, mpz_t result);
    
    /**
     * @brief H2: {0,1}* → G₁（哈希到群元素）
     */
    void computeHashH2(const std::string& input, element_t result);
    
    /**
     * @brief H3: {0,1}* → {0,1}^λ（哈希到固定长度）
     */
    std::string computeHashH3(const std::string& input);
    
    /**
     * @brief 加密关键词
     * @param keyword 关键词
     * @return 搜索令牌 Ti = H3(H1(mk || keyword))
     */
    std::string encryptKeyword(const std::string& keyword);
    
    /**
     * @brief 生成随机状态值
     */
    std::string generateRandomState();
    
    /**
     * @brief 加密指针
     * @param previous_state 前一个状态（为空则加密当前状态）
     * @param current_state_hash 当前状态的哈希值（作为密钥）
     * @return 加密后的指针
     */
    std::string encryptPointer(const std::string& previous_state, 
                              const std::string& current_state_hash);
    
    // ============ 辅助函数 ============
    
    bool readFile(const std::string& file_path, 
                 std::vector<unsigned char>& data);
    
    bool writeFile(const std::string& file_path,
                  const std::vector<unsigned char>& data);
    
    std::vector<std::vector<unsigned char>> splitIntoBlocks(
        const std::vector<unsigned char>& data, 
        size_t block_size);
    
    std::string serializeElement(element_t elem);
    
    bool deserializeElement(const std::string& hex_str, element_t elem);
    
    std::string bytesToHex(const std::vector<unsigned char>& bytes);
    
    std::string getCurrentTimestamp();
    
    // ============ 成员变量 ============
    
    // 配对参数和公共参数
    pairing_t pairing_;     // 配对结构
    element_t g_;           // 生成元（从public_params加载）
    element_t mu_;          // 认证参数（从public_params加载）
    mpz_t N_;               // RSA模数（从public_params加载）
    bool initialized_;      // 初始化状态标志
    
    // 客户端密钥
    unsigned char mk_[32];  // 主密钥（随机生成）
    mpz_t sk_;              // 私钥（随机生成）
    unsigned char ek_[32];  // 加密密钥（随机生成）
    element_t pk_;          // 公钥（计算得到：pk = g^sk）
    
    // 关键词状态管理（前向安全）
    std::map<std::string, std::string> keyword_states_;  // 关键词->当前状态
    std::string keyword_states_file_;    // 当前加载的状态文件路径
    bool states_loaded_;                 // 状态文件是否已加载
    Json::Value keyword_states_data_;    // 存储完整的JSON数据
    
    // 常量定义
    inline static constexpr size_t BLOCK_SIZE = 4096;
    inline static constexpr size_t SECTOR_SIZE = 256;
    inline static constexpr size_t SECTORS_PER_BLOCK = BLOCK_SIZE / SECTOR_SIZE;
};

#endif // DECENTRALIZED_STORAGE_CLIENT_H