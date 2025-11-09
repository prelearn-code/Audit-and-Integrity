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
 * @brief 本地加密存储客户端
 * 实现可验证的可搜索加密与前向安全性
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
     * @brief 生成客户端密钥
     * @return 成功返回true
     */
    bool generateKeys();
    
    /**
     * @brief 加密文件并生成元数据
     * @param file_path 输入文件路径
     * @param keywords 关键词列表
     * @param output_prefix 输出文件前缀（生成 prefix.enc 和 prefix.json）
     * @return 成功返回true
     */
    bool encryptFile(const std::string& file_path, 
                     const std::vector<std::string>& keywords,
                     const std::string& output_prefix);
    
    /**
     * @brief 解密文件
     * @param encrypted_file 加密文件路径
     * @param output_path 输出文件路径
     * @return 成功返回true
     */
    bool decryptFile(const std::string& encrypted_file, 
                     const std::string& output_path);
    
    /**
     * @brief 生成搜索令牌
     * @param keyword 关键词
     * @param output_file 输出JSON文件路径
     * @return 成功返回true
     */
    bool generateSearchToken(const std::string& keyword,
                            const std::string& output_file);
    
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
    
    // ============ 新增：关键词状态管理功能 ============
    
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
     * @param param_file 参数文件路径（默认为"system_params.json"）
     * @return 成功返回true
     */
    bool loadSystemParams(const std::string& param_file = "/home/zsw/codes/Audit-and-Integrity/vds-client/data/system_params.json");
    
    // 密码学操作
    
    bool encryptFileData(const std::vector<unsigned char>& plaintext,
                        std::vector<unsigned char>& ciphertext);
    
    bool decryptFileData(const std::vector<unsigned char>& ciphertext,
                        std::vector<unsigned char>& plaintext);
    
    bool generateAuthTags(const std::string& file_id,
                         const std::vector<unsigned char>& ciphertext,
                         std::vector<std::string>& auth_tags);
    
    bool generateKeywordTag(const std::string& file_id,
                           const std::string& keyword,
                           const std::string& current_state,
                           const std::string& previous_state,
                           std::string& kt_output);
    
    void computeHashH1(const std::string& input, mpz_t result);
    
    void computeHashH2(const std::string& input, element_t result);
    
    std::string computeHashH3(const std::string& input);
    
    std::string encryptKeyword(const std::string& keyword);
    
    std::string generateRandomState();
    
    std::string encryptPointer(const std::string& data, 
                              const std::string& key);
    
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
    
    std::string getCurrentTimestamp();  // 新增：获取当前时间戳
    
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
    
    // ============ 新增：状态管理相关变量 ============
    std::string keyword_states_file_;    // 当前加载的状态文件路径
    bool states_loaded_;                 // 状态文件是否已加载
    Json::Value keyword_states_data_;    // 存储完整的JSON数据
    
    inline static constexpr size_t BLOCK_SIZE = 4096;
    inline static constexpr size_t SECTOR_SIZE = 256;
    inline static constexpr size_t SECTORS_PER_BLOCK = BLOCK_SIZE / SECTOR_SIZE;
};

#endif // DECENTRALIZED_STORAGE_CLIENT_H