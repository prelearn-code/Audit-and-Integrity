#ifndef STORAGE_NODE_H
#define STORAGE_NODE_H

#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <pbc/pbc.h>
#include <gmp.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>

struct IndexKeywords
{
    std::string ptr_i;   // 关键字的状态指针
    std::string kt_wi;   // 关键词关联标签
    std::string Ti_bar;  // 状态关联的Token
};

// 统一的数据结构：IndexEntry（同时用于索引和文件存储）
struct IndexEntry { 
    std::string ID_F;                      // 文件标识符 (ID_F)
    std::string PK;                        // 客户端公钥
    std::vector<std::string> TS_F;         // 文件认证标签集合
    std::string state;                     // 状态: "valid" 或 "invalid"
    std::string file_path;                 // 文件的本地存储位置
    std::vector<IndexKeywords> keywords;   // 关联信息的集合
};

// 搜索索引条目：用于快速搜索功能，以 Ti_bar 为键进行索引
struct IndexSearchEntry {
    std::string Ti_bar;    // 插入文件的状态令牌（作为唯一键）
    std::string ID_F;      // 文件ID
    std::string ptr_i;     // 关键词状态指针
    std::string state;     // 文件状态: "valid" 或 "invalid"
    std::string kt_wi;     // 关键词关联标签
};

// 修改后的SearchResult结构体（用于中间搜索过程）
struct SearchResult {
    std::string ID_F;      // 文件ID
    std::string psi;       // ψ值（累积证明）
    std::string phi;       // φ值（累积签名）
};

// 文件证明结构体


struct FileProof {
    std::string psi;   // ψ值（累积证明）
    std::string phi;   // φ值（累积签名）
};
class StorageNode {
public:
    // 文件分块常量
    static constexpr size_t BLOCK_SIZE = 4096;        // 加密文件分块大小（字节）
    static constexpr size_t SECTOR_SIZE = 256;        // 扇区大小（字节）
    static constexpr size_t SECTORS_PER_BLOCK = BLOCK_SIZE / SECTOR_SIZE;  // 每块扇区数 = 16
    
    // 密码学参数
    pairing_t pairing;
    element_t g;
    element_t mu;
    mpz_t N;
    bool crypto_initialized;
    
    // 存储（统一使用IndexEntry，以ID_F为键）
    std::map<std::string, IndexEntry> index_database;
    
    // 搜索索引数据库（以 Ti_bar 为键，用于快速搜索）
    std::map<std::string, IndexSearchEntry> search_database;
    
    // 配置
    std::string node_id;
    std::string data_dir;
    std::string files_dir;
    std::string metadata_dir;
    std::string FileProofs_dir;
    std::string SearchProof_dir;
    int server_port;
    
    // 辅助函数
    std::string generate_random_seed();
    
    // JSON文件操作
    Json::Value load_json_from_file(const std::string& filepath);
    bool save_json_to_file(const Json::Value& root, const std::string& filepath);
    
    // 文件系统操作
    std::string read_file_content(const std::string& filepath);
    bool write_file_content(const std::string& filepath, const std::string& content);
    bool file_exists(const std::string& filepath) const;
    bool create_directory(const std::string& dirpath);
    std::string get_current_timestamp();
    
    // 身份验证
    /**
    *@brief 验证客户端公钥格式
    *@param pk 客户端公钥字符串
    *@return 验证通过返回true，失败返回false
    */
    bool verify_pk_format(const std::string& pk);

// public
    StorageNode(const std::string& data_directory = "../data", int port = 9000);
    ~StorageNode();
    
    // ========== 初始化 ==========
    
    bool setup_cryptography(int security_param, 
                           const std::string& public_params_path = "");
    bool save_public_params(const std::string& filepath);
    bool load_public_params(const std::string& filepath);
    bool display_public_params(const std::string& filepath = "");
    bool initialize_directories();
    bool load_config();
    bool save_config();
    bool create_default_config();
    
    // ========== 索引数据库操作 ==========
    
    bool load_index_database();
    bool save_index_database();
    bool load_search_database();
    bool save_search_database();
    
    // ========== 节点信息 ==========
    
    bool load_node_info();
    bool save_node_info();
    void update_statistics(const std::string& operation);
    
    // ========== 文件操作 ==========
    
    bool insert_file(const std::string& param_json_path, const std::string& enc_file_path);
    bool delete_file(const std::string& PK, const std::string& file_id, const std::string& del_proof);
    
    // ========== 新增功能 ==========
    
    /**
     * delete_file_from_json() - 从JSON文件删除文件
     * @param delete_json_path 删除参数JSON文件路径
     * @return 成功返回true，失败返回false
     */
    bool delete_file_from_json(const std::string& delete_json_path);
    
    /**
     * SearchKeywordsAssociatedFilesProof() - 搜索关键词关联文件证明
     * @param search_json_path 搜索参数JSON文件路径
     * @return 成功返回true，失败返回false
     */
    bool SearchKeywordsAssociatedFilesProof(const std::string& search_json_path);
    
    /**
     * GetFileProof() - 获取文件证明
     * @param ID_F 文件ID
     * @return 成功返回true，失败返回false
     */
    bool GetFileProof(const std::string& ID_F);
    
    /**
     * VerifySearchProof() - 验证搜索证明
     * @param search_proof_json_path 搜索证明JSON文件路径
     * @return 验证成功返回true，失败返回false
     */
    bool VerifySearchProof(const std::string& search_proof_json_path);
    
    /**
     * VerifyFileProof() - 验证文件证明
     * @param file_proof_json_path 文件证明JSON文件路径
     * @return 验证成功返回true，失败返回false
     */
    bool VerifyFileProof(const std::string& file_proof_json_path);
    
    // ========== 检索函数 ==========
    
    Json::Value retrieve_file(const std::string& file_id);
    Json::Value get_file_metadata(const std::string& file_id);
    bool export_file_metadata(const std::string& file_id, const std::string& output_path);
    
    // ========== 文件存储 ==========
    
    bool save_encrypted_file(const std::string& file_id, const std::string& enc_file_path);
    bool load_encrypted_file(const std::string& file_id, std::string& ciphertext);
    std::vector<std::string> list_all_files();
    std::vector<std::string> list_files_by_pk(const std::string& PK);

    // 辅助函数（统一驼峰命名）
    std::string bytesToHex(const unsigned char* data, size_t len);
    std::vector<unsigned char> hexToBytes(const std::string& hex);
    

    // ========== Getters ==========
    
    std::string get_node_id() const {
        return node_id;
    }
    
    std::string get_data_dir() const {
        return data_dir;
    }
    
    int get_server_port() const {
        return server_port;
    }
    
    size_t get_file_count() const {
        return index_database.size();
    }
    
    size_t get_index_count() const {
        return index_database.size();
    }
    
    size_t get_search_index_count() const {
        return search_database.size();
    }
    
    bool has_file(const std::string& file_id) const {
        return index_database.find(file_id) != index_database.end();
    }
    
    bool is_crypto_initialized() const {
        return crypto_initialized;
    }
    
    bool has_public_params_file(const std::string& filepath) const;
    
    // ========== 状态显示 ==========
    
    void print_status() const {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "📊 存储节点状态" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "节点 ID:      " << node_id << std::endl;
        std::cout << "数据目录:     " << data_dir << std::endl;
        std::cout << "端口:         " << server_port << std::endl;
        std::cout << "文件数:       " << get_index_count() << std::endl;
        std::cout << "密码学:       " << (crypto_initialized ? "已初始化" : "未初始化") << std::endl;
        std::cout << "版本:         v3.8 (统一序列化函数+错误检查)" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    void print_detailed_status();

    // 密码学函数（修改为void返回值）
    void computeHashH1(const std::string& input, mpz_t result);
    void computeHashH2(const std::string& input, element_t result);
    std::string computeHashH3(const std::string& input);
    void compute_prf(mpz_t result, const std::string& seed, const std::string& ID_F, int index);
    std::string decrypt_pointer(const std::string& current_state_hash, const std::string& encrypted_pointer);
    
    // 序列化辅助函数（与client.cpp统一，方案A核心修改）
    std::string serializeElement(element_t elem);
    bool deserializeElement(const std::string& hex_str, element_t elem);
};

#endif // STORAGE_NODE_H
