#ifndef STORAGE_NODE_H
#define STORAGE_NODE_H

#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
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
#include <json/json.h>

struct IndexEntry {
    std::string PK;              // 客户端公钥 (新增)
    std::string Ts;              // 状态令牌 (T_i)
    std::string keyword;         // 关键词 (kt_i)
    std::string pointer;         // 加密指针
    std::string file_identifier; // 文件标识符 (ID_F)
    std::string state;           // 状态: "valid" 或 "invalid" (修改为string)
};

struct FileData {
    std::string PK;              // 客户端公钥 (新增)
    std::string file_id;         // 文件ID (ID_F)
    std::string ciphertext;      // 加密文本
    std::string pointer;         // 文件指针 (ptr)
    std::string file_auth_tag;   // 文件认证标签 (TS_F)
    std::string state;           // 状态: "valid" 或 "invalid" (新增)
};

struct SearchResult {
    std::vector<std::string> file_identifiers;
    std::vector<std::string> keyword_proofs;
    std::string aggregated_proof;
};

/**
 * StorageNode - 去中心化存储节点 (本地版 v3.4)
 * 
 * 特性:
 * - ✅ 完全本地化存储
 * - ✅ JSON文件持久化
 * - ✅ 交互式控制台
 * - ✅ 无区块链依赖
 * - ✅ 客户端公钥身份验证 (v3.1新增)
 * - ✅ 文件状态管理 (v3.1新增)
 * - ✅ 简化公共参数 N,g,μ (v3.2新增)
 * - ✅ 灵活的密码学初始化 (v3.3新增)
 * - ✅ 改进的参数序列化 element_to_bytes (v3.4新增)
 * - ✅ 向后兼容旧格式 (v3.4新增)
 */
class StorageNode {
private:
    // 密码学参数
    pairing_t pairing;
    element_t g;
    element_t mu;
    mpz_t N;
    bool crypto_initialized;
    
    // 存储
    std::map<std::string, std::vector<IndexEntry>> index_database;
    std::map<std::string, FileData> file_storage;
    
    // 配置
    std::string node_id;
    std::string data_dir;
    std::string files_dir;
    std::string metadata_dir;
    int server_port;
    
    // 密码学函数
    std::string compute_hash_H1(const std::string& input);
    void compute_hash_H2(element_t result, const std::string& input);
    std::string compute_hash_H3(const std::string& input);
    void compute_prf(mpz_t result, const std::string& seed, const std::string& input);
    std::string decrypt_pointer(const std::string& encrypted_pointer, const std::string& key);
    
    // JSON文件操作
    Json::Value load_json_from_file(const std::string& filepath);
    bool save_json_to_file(const Json::Value& root, const std::string& filepath);
    
    // 文件系统操作
    std::string read_file_content(const std::string& filepath);
    bool write_file_content(const std::string& filepath, const std::string& content);
    bool file_exists(const std::string& filepath) const;
    bool create_directory(const std::string& dirpath);
    std::string get_current_timestamp();
    
    // 辅助函数
    std::string bytes_to_hex(const unsigned char* data, size_t len);
    std::vector<unsigned char> hex_to_bytes(const std::string& hex);
    
    // 身份验证 (v3.1新增)
    bool verify_pk_format(const std::string& pk);

public:
    StorageNode(const std::string& data_directory = "./data", int port = 9000);
    ~StorageNode();
    
    // ========== 初始化 ==========
    
    /**
     * setup_cryptography() - 初始化密码学参数并生成公共参数
     * @param security_param 安全参数K（比特位数，如512）
     * @param public_params_path 公共参数保存路径（可选，为空则不保存）
     * @return 成功返回true，失败返回false
     * 
     * 生成公共参数 PP = {N, g, μ}，其中 N = p × q
     */
    bool setup_cryptography(int security_param, 
                           const std::string& public_params_path = "");
    
    /**
     * save_public_params() - 保存公共参数到JSON文件
     * @param filepath 公共参数文件保存路径
     * @return 成功返回true，失败返回false
     * 
     * 保存内容：N, g, μ（只保存这三个核心参数）
     */
    bool save_public_params(const std::string& filepath);
    
    /**
     * load_public_params() - 从JSON文件加载公共参数并初始化密码学系统
     * @param filepath 公共参数文件路径
     * @return 成功返回true，失败返回false
     * 
     * 功能：
     * 1. 从JSON文件读取公共参数 (N, g, μ)
     * 2. 在控制台显示参数信息
     * 3. 初始化密码学系统并恢复状态
     * 4. 设置 crypto_initialized = true
     * 
     * 用于：节点启动时加载已有参数，快速恢复密码学状态
     */
    bool load_public_params(const std::string& filepath);
    
    /**
     * display_public_params() - 显示已加载的公共参数（只读操作）
     * @param filepath 公共参数文件路径（可选，若为空则显示内存中的参数）
     * @return 成功返回true，失败返回false
     * 
     * 功能：
     * 1. 如果提供filepath，从JSON文件读取并显示参数信息
     * 2. 如果filepath为空且crypto_initialized=true，显示内存中的参数
     * 3. 纯查看功能，不会修改密码学系统状态
     * 
     * 用于：用户查看公共参数，不会触发重新加载
     */
    bool display_public_params(const std::string& filepath = "");
    
    /**
     * initialize_directories() - 初始化数据目录
     */
    bool initialize_directories();
    
    /**
     * load_config() - 加载配置文件
     */
    bool load_config();
    
    /**
     * save_config() - 保存配置文件
     */
    bool save_config();
    
    /**
     * create_default_config() - 创建默认配置
     */
    bool create_default_config();
    
    // ========== 索引数据库操作 ==========
    
    /**
     * load_index_database() - 从文件加载索引数据库
     */
    bool load_index_database();
    
    /**
     * save_index_database() - 保存索引数据库到文件
     */
    bool save_index_database();
    
    // ========== 节点信息 ==========
    
    /**
     * load_node_info() - 加载节点信息
     */
    bool load_node_info();
    
    /**
     * save_node_info() - 保存节点信息
     */
    bool save_node_info();
    
    /**
     * update_statistics() - 更新统计信息
     */
    void update_statistics(const std::string& operation);
    
    // ========== 文件操作 (v3.1修改) ==========
    
    /**
     * insert_file() - 插入文件 (v3.1: 使用新的JSON格式)
     * @param param_json_path 参数JSON文件路径，包含PK, ID_F, ptr, TS_F, state, keywords等
     * @param enc_file_path 加密文件路径
     * 
     * JSON格式:
     * {
     *   "PK": "客户端公钥",
     *   "ID_F": "文件唯一标识",
     *   "ptr": "文件指针",
     *   "TS_F": "文件认证标签",
     *   "state": "valid",
     *   "keywords": [
     *     {"T_i": "状态令牌1", "kt_i": "关键词1"},
     *     {"T_i": "状态令牌2", "kt_i": "关键词2"}
     *   ]
     * }
     */
    bool insert_file(const std::string& param_json_path, const std::string& enc_file_path);
    
    /**
     * delete_file() - 删除文件 (v3.1: 增加PK身份验证)
     * @param PK 客户端公钥，用于身份验证
     * @param file_id 文件ID
     * @param del_proof 删除证明
     */
    bool delete_file(const std::string& PK, const std::string& file_id, const std::string& del_proof);
    
    /**
     * search_keyword() - 搜索关键词 (v3.1: 增加PK过滤)
     * @param PK 客户端公钥，只返回该客户端的文件
     * @param search_token 搜索令牌
     * @param latest_state 最新状态
     * @param seed 种子
     */
    SearchResult search_keyword(const std::string& PK,
                               const std::string& search_token, 
                               const std::string& latest_state,
                               const std::string& seed);
    
    /**
     * generate_integrity_proof() - 生成完整性证明
     */
    std::string generate_integrity_proof(const std::string& file_id, 
                                        const std::string& seed);
    
    // ========== 检索函数 ==========
    
    /**
     * retrieve_file() - 检索文件
     */
    Json::Value retrieve_file(const std::string& file_id);
    
    /**
     * retrieve_files_batch() - 批量检索文件
     */
    Json::Value retrieve_files_batch(const std::vector<std::string>& file_ids);
    
    /**
     * get_file_metadata() - 获取文件元数据
     */
    Json::Value get_file_metadata(const std::string& file_id);
    
    /**
     * export_file_metadata() - 导出文件元数据到JSON
     */
    bool export_file_metadata(const std::string& file_id, const std::string& output_path);
    
    // ========== 文件存储 ==========
    
    /**
     * save_encrypted_file() - 保存加密文件到文件系统
     */
    bool save_encrypted_file(const std::string& file_id, const std::string& enc_file_path);
    
    /**
     * load_encrypted_file() - 从文件系统加载加密文件
     */
    bool load_encrypted_file(const std::string& file_id, std::string& ciphertext);
    
    /**
     * list_all_files() - 列出所有文件
     */
    std::vector<std::string> list_all_files();
    
    /**
     * list_files_by_pk() - 列出指定PK的所有文件 (v3.1新增)
     */
    std::vector<std::string> list_files_by_pk(const std::string& PK);
    
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
        return file_storage.size();
    }
    
    size_t get_index_count() const {
        size_t count = 0;
        for (const auto& entry : index_database) {
            count += entry.second.size();
        }
        return count;
    }
    
    bool has_file(const std::string& file_id) const {
        return file_storage.find(file_id) != file_storage.end();
    }
    
    bool is_crypto_initialized() const {
        return crypto_initialized;
    }
    
    /**
     * has_public_params_file() - 检查公共参数文件是否存在
     * @param filepath 公共参数文件路径
     * @return 文件存在返回true，否则返回false
     * 
     * 用于：在启动时检测是否已有公共参数文件，以决定是加载还是初始化
     */
    bool has_public_params_file(const std::string& filepath) const;
    
    // ========== 状态显示 ==========
    
    void print_status() const {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "📊 存储节点状态" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "节点 ID:      " << node_id << std::endl;
        std::cout << "数据目录:     " << data_dir << std::endl;
        std::cout << "端口:         " << server_port << std::endl;
        std::cout << "文件数:       " << file_storage.size() << std::endl;
        std::cout << "索引数:       " << get_index_count() << std::endl;
        std::cout << "密码学:       " << (crypto_initialized ? "已初始化" : "未初始化") << std::endl;
        std::cout << "版本:         v3.4 (改进的参数序列化)" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    void print_detailed_status();
};

#endif // STORAGE_NODE_H