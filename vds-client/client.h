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
 * @brief 本地加密存储客户端 v3.3
 * 实现可验证的可搜索加密与前向安全性
 * 
 * v3.3 主要变更：
 * - generateKeys() 从 public_params.json 读取参数，生成 public_key.json
 * - encryptFile() 生成 insert.json 和本地元数据
 * - 删除 generateSearchToken()，与 encryptKeyword() 合并
 * - generateAuthTags() 算法重写（按论文实现）
 * - generateKeywordTag() 改名为 generateStateAssociatedToken()
 * - encryptPointer() 调整逻辑
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
     * @brief 初始化客户端（本地配对参数）
     * @return 成功返回true
     */
    bool initialize();
    
    /**
     * @brief 生成客户端密钥（v3.3修改）
     * @param public_params_file Storage Node生成的公共参数文件路径
     * @return 成功返回true
     * 
     * 新版本会：
     * 1. 从 public_params.json 读取参数
     * 2. 生成 private_key.dat（二进制）
     * 3. 生成 public_key.json（JSON格式）
     */
    bool generateKeys(const std::string& public_params_file = "public_params.json");
    
    /**
     * @brief 加密文件并生成元数据（v3.3修改）
     * @param file_path 输入文件路径
     * @param keywords 关键词列表
     * @param output_prefix 输出文件前缀
     * @param insert_json_path 生成的insert.json路径（供Storage Node使用）
     * @return 成功返回true
     * 
     * 新版本输出：
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
     * @brief 从文件加载密钥
     * @param key_file 密钥文件路径
     * @return 成功返回true
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
    // 系统参数加载
    
    /**
     * @brief 从JSON文件加载系统参数
     * @param param_file 参数文件路径
     * @return 成功返回true
     */
    bool loadSystemParams(const std::string& param_file = "/home/zsw/codes/Audit-and-Integrity/vds-client/data/system_params.json");
    
    // 密码学操作
    
    bool encryptFileData(const std::vector<unsigned char>& plaintext,
                        std::vector<unsigned char>& ciphertext);
    
    bool decryptFileData(const std::vector<unsigned char>& ciphertext,
                        std::vector<unsigned char>& plaintext);
    
    /**
     * @brief 生成认证标签（v3.3重写）
     * @param file_id 文件ID
     * @param ciphertext 密文
     * @param auth_tags 输出的认证标签
     * @return 成功返回true
     * 
     * 新算法：σ_i = [H_2(ID_F||i) * ∏_{j=1}^s μ^{c_{i,j}}]^sk
     */
    bool generateAuthTags(const std::string& file_id,
                         const std::vector<unsigned char>& ciphertext,
                         std::vector<std::string>& auth_tags);
    
    /**
     * @brief 生成状态关联令牌（v3.3改名和算法修正）
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
    
    void computeHashH1(const std::string& input, mpz_t result);
    
    void computeHashH2(const std::string& input, element_t result);
    
    std::string computeHashH3(const std::string& input);
    
    /**
     * @brief 加密关键词（v3.3简化）
     * @param keyword 关键词
     * @return 搜索令牌 Ti = H3(H1(mk || keyword))
     */
    std::string encryptKeyword(const std::string& keyword);
    
    std::string generateRandomState();
    
    /**
     * @brief 加密指针（v3.3调整）
     * @param previous_state 前一个状态（为空则加密当前状态）
     * @param current_state_hash 当前状态的哈希值（作为密钥）
     * @return 加密后的指针
     */
    std::string encryptPointer(const std::string& previous_state, 
                              const std::string& current_state_hash);
    
    // 辅助函数
    
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
    
    // 成员变量
    
    pairing_t pairing_;
    element_t g_;
    element_t mu_;
    mpz_t N_;
    bool initialized_;
    
    unsigned char mk_[32];  // 主密钥
    mpz_t sk_;              // 私钥
    unsigned char ek_[32];  // 加密密钥
    element_t pk_;          // 公钥
    
    std::map<std::string, std::string> keyword_states_;  // 关键词状态（前向安全）
    
    // 状态管理相关变量
    std::string keyword_states_file_;    // 当前加载的状态文件路径
    bool states_loaded_;                 // 状态文件是否已加载
    Json::Value keyword_states_data_;    // 存储完整的JSON数据
    
    inline static constexpr size_t BLOCK_SIZE = 4096;
    inline static constexpr size_t SECTOR_SIZE = 256;
    inline static constexpr size_t SECTORS_PER_BLOCK = BLOCK_SIZE / SECTOR_SIZE;
};

#endif // DECENTRALIZED_STORAGE_CLIENT_H