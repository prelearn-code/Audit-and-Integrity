#include "./insert_test.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
const char* kDefaultConfigPath = "config/insert_test_config.json";
}

// ==================== 构造函数和析构函数 ====================

InsertPerformanceTest::InsertPerformanceTest()
    : client_(nullptr),
      server_(nullptr),
      max_files_(0),
      verbose_(true),
      save_intermediate_(true),
      server_port_(9000) {
    
    // 设置性能监控回调
    callback_s.on_phase_complete = [this](const std::string& name, double time_ms) {
        current_times_[name] = time_ms;
        if (verbose_) {
            std::cout << "  [TIME] " << name << ": " << time_ms << " ms" << std::endl;
        }
    };
    
    callback_s.on_data_size_recorded = [this](const std::string& name, size_t size_bytes) {
        current_sizes_[name] = size_bytes;
        if (verbose_) {
            std::cout << "  [SIZE] " << name << ": " << size_bytes << " bytes" << std::endl;
        }
    };
    callback_c.on_phase_complete = [this](const std::string& name, double time_ms) {
        current_times_[name] = time_ms;
        if (verbose_) {
            std::cout << "  [TIME] " << name << ": " << time_ms << " ms" << std::endl;
        }
    };
    
    callback_c.on_data_size_recorded = [this](const std::string& name, size_t size_bytes) {
        current_sizes_[name] = size_bytes;
        if (verbose_) {
            std::cout << "  [SIZE] " << name << ": " << size_bytes << " bytes" << std::endl;
        }
    };
}

InsertPerformanceTest::~InsertPerformanceTest() {
    if (client_) delete client_;
    if (server_) delete server_;
}

// ==================== 配置加载 ====================

bool InsertPerformanceTest::loadConfig(const std::string& config_file) {
    std::cout << "\n[配置] 加载测试配置: " << config_file << std::endl;
    
    // 读取JSON配置文件
    std::ifstream ifs(config_file);
    if (!ifs.is_open()) {
        std::cerr << "[错误] 无法打开配置文件: " << config_file << std::endl;
        return false;
    }
    
    Json::Value config;
    Json::CharReaderBuilder reader;
    std::string errs;
    
    if (!Json::parseFromStream(reader, ifs, &config, &errs)) {
        std::cerr << "[错误] JSON解析失败: " << errs << std::endl;
        return false;
    }
    
    // 提取路径配置
    const Json::Value& paths = config["paths"];
    keywords_file_ = paths.get("keywords_file", "").asString();
    base_dir_ = paths.get("dataset_root", "").asString();
    public_params_file_ = paths.get("public_params", "").asString();
    private_key_file_ = paths.get("private_key", "private_key.dat").asString();
    
    const Json::Value& client_cfg = paths["client"];
    client_data_dir_ = client_cfg.get("data_dir", "data").asString();
    client_insert_dir_ = client_cfg.get("insert_dir", client_data_dir_ + "/Insert").asString();
    client_enc_dir_ = client_cfg.get("enc_dir", client_data_dir_ + "/EncFiles").asString();
    client_meta_dir_ = client_cfg.get("metadata_dir", client_data_dir_ + "/MetaFiles").asString();
    client_search_dir_ = client_cfg.get("search_dir", client_data_dir_ + "/Search").asString();
    client_deles_dir_ = client_cfg.get("deles_dir", client_data_dir_ + "/Deles").asString();
    keyword_states_file_ = client_cfg.get("keyword_states_file", client_data_dir_ + "/keyword_states.json").asString();
    
    const Json::Value& server_cfg = paths["server"];
    server_data_dir_ = server_cfg.get("data_dir", "Storage-node/data").asString();
    server_insert_dir_ = server_cfg.get("insert_dir", client_insert_dir_).asString();
    server_enc_dir_ = server_cfg.get("enc_dir", client_enc_dir_).asString();
    server_port_ = server_cfg.get("port", 9000).asInt();
    
    // 提取选项
    const Json::Value& options = config["options"];
    max_files_ = options.get("max_files", 0).asInt();
    verbose_ = options.get("verbose", true).asBool();
    save_intermediate_ = options.get("save_intermediate", true).asBool();
    
    statistics_.test_name = config.get("test_name", "insert_performance").asString();
    
    std::cout << "[配置] 关键词文件: " << keywords_file_ << std::endl;
    std::cout << "[配置] 数据根目录: " << base_dir_ << std::endl;
    std::cout << "[配置] 客户端数据目录: " << client_data_dir_ << std::endl;
    std::cout << "[配置] 客户端密钥: " << private_key_file_ << std::endl;
    std::cout << "[配置] 服务端数据目录: " << server_data_dir_ << std::endl;
    std::cout << "[配置] 最大文件数: " << (max_files_ > 0 ? std::to_string(max_files_) : "全部") << std::endl;
    
    // 规范化路径，便于后续检查
    keywords_file_ = fs::path(keywords_file_).lexically_normal().string();
    base_dir_ = fs::path(base_dir_).lexically_normal().string();
    public_params_file_ = fs::path(public_params_file_).lexically_normal().string();
    private_key_file_ = fs::path(private_key_file_).lexically_normal().string();
    client_data_dir_ = fs::path(client_data_dir_).lexically_normal().string();
    client_insert_dir_ = fs::path(client_insert_dir_).lexically_normal().string();
    client_enc_dir_ = fs::path(client_enc_dir_).lexically_normal().string();
    client_meta_dir_ = fs::path(client_meta_dir_).lexically_normal().string();
    client_search_dir_ = fs::path(client_search_dir_).lexically_normal().string();
    client_deles_dir_ = fs::path(client_deles_dir_).lexically_normal().string();
    keyword_states_file_ = fs::path(keyword_states_file_).lexically_normal().string();
    server_data_dir_ = fs::path(server_data_dir_).lexically_normal().string();
    server_insert_dir_ = fs::path(server_insert_dir_).lexically_normal().string();
    server_enc_dir_ = fs::path(server_enc_dir_).lexically_normal().string();
    
    if (!fs::exists(keywords_file_)) {
        std::cerr << "[错误] 关键词文件不存在: " << keywords_file_ << std::endl;
        return false;
    }
    
    return true;
}

bool InsertPerformanceTest::loadKeywordsMapping() {
    std::cout << "\n[数据] 加载文件-关键词映射..." << std::endl;
    
    std::ifstream ifs(keywords_file_);
    if (!ifs.is_open()) {
        std::cerr << "[错误] 无法打开关键词文件: " << keywords_file_ << std::endl;
        return false;
    }
    
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;
    
    if (!Json::parseFromStream(reader, ifs, &root, &errs)) {
        std::cerr << "[错误] JSON解析失败: " << errs << std::endl;
        return false;
    }
    
    // 解析文件列表
    const Json::Value& files = root["files"];
    if (files.isArray()) {
        for (const auto& file_entry : files) {
            std::string path = resolveFilePath(file_entry["path"].asString());
            std::vector<std::string> keywords;
            
            const Json::Value& kw_array = file_entry["keywords"];
            for (const auto& kw : kw_array) {
                keywords.push_back(kw.asString());
            }
            
            file_keywords_map_[path] = keywords;
        }
    } else if (root.isObject()) {
        // 支持平铺的 path -> keyword(s) 映射
        for (const auto& name : root.getMemberNames()) {
            std::string path = resolveFilePath(name);
            std::vector<std::string> keywords;
            const Json::Value& kw_value = root[name];
            
            if (kw_value.isArray()) {
                for (const auto& kw : kw_value) {
                    keywords.push_back(kw.asString());
                }
            } else {
                keywords.push_back(kw_value.asString());
            }
            file_keywords_map_[path] = keywords;
        }
    } else {
        std::cerr << "[错误] 未找到有效的文件映射字段" << std::endl;
        return false;
    }
    
    std::cout << "[数据] 已加载 " << file_keywords_map_.size() << " 个文件映射" << std::endl;
    
    return true;
}

// ==================== 初始化 ====================

bool InsertPerformanceTest::initialize() {
    std::cout << "\n[初始化] 开始初始化测试环境..." << std::endl;
    
    // 1. 加载文件-关键词映射
    if (!loadKeywordsMapping()) {
        return false;
    }
    
    // 2. 初始化客户端
    std::cout << "[初始化] 创建客户端..." << std::endl;
    client_ = new StorageClient();
    
    // 配置客户端数据目录，确保与服务端读取路径一致
    StorageClient::configureDataDirectories(
        client_data_dir_,
        client_insert_dir_,
        client_enc_dir_,
        client_meta_dir_,
        client_search_dir_,
        client_deles_dir_,
        keyword_states_file_);
    
    if (!client_->initialize(public_params_file_)) {
        std::cerr << "[错误] 客户端初始化失败" << std::endl;
        return false;
    }
    
    if (!client_->initializeDataDirectories()) {
        std::cerr << "[错误] 客户端目录初始化失败" << std::endl;
        return false;
    }
    
    // 加载或生成密钥
    fs::path key_dir = fs::path(private_key_file_).parent_path();
    if (!key_dir.empty()) {
        fs::create_directories(key_dir);
    }
    if (!client_->loadKeys(private_key_file_)) {
        std::cout << "[初始化] 未找到密钥，生成新密钥..." << std::endl;
        if (!client_->generateKeys(private_key_file_)) {
            std::cerr << "[错误] 密钥生成失败" << std::endl;
            return false;
        }
        // 确保保存到配置指定的位置
        client_->saveKeys(private_key_file_);
    }
    
    // 设置性能监控回调
    client_->setPerformanceCallback_c(&callback_c);
    
    // 3. 初始化服务端
    std::cout << "[初始化] 创建服务端..." << std::endl;
    server_ = new StorageNode(server_data_dir_, server_port_);
    
    if (!server_->load_public_params(public_params_file_)) {
        std::cerr << "[错误] 服务端加载公共参数失败" << std::endl;
        return false;
    }
    
    if (!server_->initialize_directories()) {
        std::cerr << "[错误] 服务端目录初始化失败" << std::endl;
        return false;
    }
    
    // 加载数据库（如果存在）
    server_->load_index_database();
    server_->load_search_database();
    
    // 设置性能监控回调
    server_->setPerformanceCallback_s(&callback_s);
    
    std::cout << "[初始化] 客户端Insert目录: " << client_insert_dir_ << std::endl;
    std::cout << "[初始化] 客户端密文目录: " << client_enc_dir_ << std::endl;
    std::cout << "[初始化] 服务端参数目录: " << server_data_dir_ << std::endl;
    if (server_insert_dir_ != client_insert_dir_) {
        std::cout << "[提示] 服务端插入参数将从 " << server_insert_dir_ 
                  << " 读取，与客户端生成位置不同" << std::endl;
    }
    if (server_enc_dir_ != client_enc_dir_) {
        std::cout << "[提示] 服务端密文将从 " << server_enc_dir_ 
                  << " 读取，与客户端生成位置不同" << std::endl;
    }
    
    std::cout << "[初始化] ✅ 初始化完成" << std::endl;
    
    return true;
}

bool InsertPerformanceTest::cleanupData() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🧹 清理所有数据库和测试数据" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    namespace fs = std::filesystem;

    // 清理客户端产生的文件
    std::cout << "[清理] 清理客户端数据..." << std::endl;

    // 清理加密文件
    if (fs::exists(client_enc_dir_)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(client_enc_dir_)) {
            if (entry.is_regular_file()) {
                fs::remove(entry.path());
                count++;
            }
        }
        std::cout << "  ✅ 删除加密文件: " << count << " 个" << std::endl;
    }

    // 清理元数据文件
    if (fs::exists(client_meta_dir_)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(client_meta_dir_)) {
            if (entry.is_regular_file()) {
                fs::remove(entry.path());
                count++;
            }
        }
        std::cout << "  ✅ 删除元数据文件: " << count << " 个" << std::endl;
    }

    // 清理插入JSON文件
    if (fs::exists(client_insert_dir_)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(client_insert_dir_)) {
            if (entry.is_regular_file()) {
                fs::remove(entry.path());
                count++;
            }
        }
        std::cout << "  ✅ 删除插入JSON文件: " << count << " 个" << std::endl;
    }

    // 清理关键词状态文件
    if (fs::exists(keyword_states_file_)) {
        fs::remove(keyword_states_file_);
        std::cout << "  ✅ 删除关键词状态文件" << std::endl;
    }

    // 清理服务端数据库
    std::cout << "[清理] 清理服务端数据..." << std::endl;

    // 清理索引数据库文件
    std::string index_db = server_data_dir_ + "/index.json";
    if (fs::exists(index_db)) {
        fs::remove(index_db);
        std::cout << "  ✅ 删除索引数据库: index.json" << std::endl;
    }

    // 清理搜索数据库文件
    std::string search_db = server_data_dir_ + "/search.json";
    if (fs::exists(search_db)) {
        fs::remove(search_db);
        std::cout << "  ✅ 删除搜索数据库: search.json" << std::endl;
    }

    // 清理加密文件存储
    std::string server_enc = server_data_dir_ + "/EncFiles";
    if (fs::exists(server_enc)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(server_enc)) {
            if (entry.is_regular_file()) {
                fs::remove(entry.path());
                count++;
            }
        }
        std::cout << "  ✅ 删除服务端加密文件: " << count << " 个" << std::endl;
    }

    std::cout << "\n✅ 数据清理完成\n" << std::endl;
    return true;
}

// ==================== 测试执行 ====================

bool InsertPerformanceTest::runTest() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "开始插入性能测试" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    // 清理之前的数据
    if (!cleanupData()) {
        std::cerr << "❌ 数据清理失败" << std::endl;
        return false;
    }

    statistics_.start_time = getCurrentTimestamp();
    auto start = std::chrono::high_resolution_clock::now();
    
    int total = file_keywords_map_.size();
    if (max_files_ > 0 && max_files_ < total) {
        total = max_files_;
    }
    
    std::cout << "\n[测试] 将测试 " << total << " 个文件" << std::endl;
    
    int count = 0;
    for (const auto& entry : file_keywords_map_) {
        if (max_files_ > 0 && count >= max_files_) {
            break;
        }
        
        count++;
        
        std::cout << "\n" << std::string(80, '-') << std::endl;
        std::cout << "[" << count << "/" << total << "] 测试文件: " 
                  << entry.first << std::endl;
        std::cout << "关键词: ";
        for (const auto& kw : entry.second) {
            std::cout << kw << " ";
        }
        std::cout << std::endl;
        
        if (!fs::exists(entry.first)) {
            std::cerr << "⚠️  文件不存在，跳过: " << entry.first << std::endl;
            FileTestResult result;
            result.file_path = entry.first;
            result.keyword_count = entry.second.size();
            result.timestamp = getCurrentTimestamp();
            result.success = false;
            result.error_msg = "文件不存在";
            results_.push_back(result);
            printProgress(count, total);
            continue;
        }
        
        // 测试单个文件
        FileTestResult result = testSingleFile(entry.first, entry.second);
        results_.push_back(result);
        
        // 显示进度
        printProgress(count, total);
        
        if (!result.success) {
            std::cerr << "⚠️  测试失败: " << result.error_msg << std::endl;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    statistics_.end_time = getCurrentTimestamp();
    statistics_.total_duration_sec = duration.count() / 1000.0;
    statistics_.total_files = results_.size();
    
    // 计算统计数据
    calculateStatistics();
    
    // 打印总结
    printSummary();
    
    return true;
}

InsertPerformanceTest::FileTestResult InsertPerformanceTest::testSingleFile(
    const std::string& file_path, 
    const std::vector<std::string>& keywords) {
    
    FileTestResult result;
    result.file_path = file_path;
    result.keyword_count = keywords.size();
    result.timestamp = getCurrentTimestamp();
    result.success = false;
    
    // 清理上次的性能数据
    clearPerformanceData();
    
    try {
        // 步骤1：客户端加密文件
        std::cout << "  [步骤1] 客户端加密文件..." << std::endl;
        
        if (!client_->encryptFile(file_path, keywords)) {
            result.error_msg = "客户端加密失败";
            return result;
        }
        
        // 获取T1, S1, S2, S3
        result.t1_ms = current_times_["client_encrypt_total"];
        result.s1_bytes = current_sizes_["plaintext_size"];
        result.s2_bytes = current_sizes_["encrypted_file_size"];
        result.s3_bytes = current_sizes_["insert_json_size"];
        result.file_size = result.s1_bytes;
        
        // 步骤2：服务端插入文件
        std::cout << "  [步骤2] 服务端插入文件..." << std::endl;
        
        // 找到生成的insert.json和.enc文件（与客户端命名规则一致：绝对路径+分隔符替换）
        std::string safe_name = makeSafeName(file_path);
        std::string client_enc_file = client_enc_dir_ + "/" + safe_name + ".enc";
        std::string client_insert_json = client_insert_dir_ + "/" + safe_name + "_insert.json";
        std::string server_enc_file = server_enc_dir_ + "/" + safe_name + ".enc";
        std::string server_insert_json = server_insert_dir_ + "/" + safe_name + "_insert.json";
        
        std::string enc_file = fs::exists(server_enc_file) ? server_enc_file : client_enc_file;
        std::string insert_json = fs::exists(server_insert_json) ? server_insert_json : client_insert_json;
        
        if (verbose_) {
            std::cout << "    使用的insert.json路径: " << insert_json << std::endl;
            std::cout << "    使用的密文路径: " << enc_file << std::endl;
        }
        
        // 清理性能数据
        clearPerformanceData();
        
        if (!server_->insert_file(insert_json, enc_file)) {
            result.error_msg = "服务端插入失败";
            return result;
        }
        
        // 获取T3
        result.t3_ms = current_times_["server_insert_total"];
        
        // 计算衍生指标
        result.encrypt_ratio = (double)(result.s2_bytes - result.s1_bytes) / result.s1_bytes * 100.0;
        result.metadata_ratio = (double)result.s3_bytes / result.s1_bytes * 100.0;
        result.total_overhead = (double)(result.s2_bytes + result.s3_bytes - result.s1_bytes) / result.s1_bytes * 100.0;
        
        // 计算吞吐量 (MB/s)
        result.client_throughput_mbps = (result.s1_bytes / 1024.0 / 1024.0) / (result.t1_ms / 1000.0);
        result.server_throughput_mbps = (result.s2_bytes / 1024.0 / 1024.0) / (result.t3_ms / 1000.0);
        
        result.success = true;
        
        std::cout << "  ✅ 测试成功" << std::endl;
        std::cout << "     T1=" << result.t1_ms << "ms, T3=" << result.t3_ms << "ms" << std::endl;
        std::cout << "     S1=" << result.s1_bytes << "B, S2=" << result.s2_bytes << "B, S3=" << result.s3_bytes << "B" << std::endl;
        
    } catch (const std::exception& e) {
        result.error_msg = std::string("异常: ") + e.what();
    }
    
    return result;
}

// ==================== 统计计算 ====================

void InsertPerformanceTest::calculateStatistics() {
    std::cout << "\n[统计] 计算统计数据..." << std::endl;
    
    // 收集成功的结果
    std::vector<FileTestResult> success_results;
    for (const auto& r : results_) {
        if (r.success) {
            success_results.push_back(r);
        }
    }
    
    statistics_.success_count = success_results.size();
    statistics_.failure_count = results_.size() - statistics_.success_count;
    
    if (success_results.empty()) {
        std::cerr << "[警告] 没有成功的测试结果" << std::endl;
        return;
    }
    
    // 时间统计
    std::vector<double> t1_values, t3_values;
    for (const auto& r : success_results) {
        t1_values.push_back(r.t1_ms);
        t3_values.push_back(r.t3_ms);
    }
    
    statistics_.t1_avg = std::accumulate(t1_values.begin(), t1_values.end(), 0.0) / t1_values.size();
    statistics_.t1_min = *std::min_element(t1_values.begin(), t1_values.end());
    statistics_.t1_max = *std::max_element(t1_values.begin(), t1_values.end());
    statistics_.t1_stddev = calculateStdDev(t1_values, statistics_.t1_avg);
    
    statistics_.t3_avg = std::accumulate(t3_values.begin(), t3_values.end(), 0.0) / t3_values.size();
    statistics_.t3_min = *std::min_element(t3_values.begin(), t3_values.end());
    statistics_.t3_max = *std::max_element(t3_values.begin(), t3_values.end());
    statistics_.t3_stddev = calculateStdDev(t3_values, statistics_.t3_avg);
    
    // 数据大小统计
    size_t s1_sum = 0, s2_sum = 0, s3_sum = 0;
    double encrypt_ratio_sum = 0, metadata_ratio_sum = 0, total_overhead_sum = 0;
    double client_tp_sum = 0, server_tp_sum = 0;
    
    for (const auto& r : success_results) {
        s1_sum += r.s1_bytes;
        s2_sum += r.s2_bytes;
        s3_sum += r.s3_bytes;
        encrypt_ratio_sum += r.encrypt_ratio;
        metadata_ratio_sum += r.metadata_ratio;
        total_overhead_sum += r.total_overhead;
        client_tp_sum += r.client_throughput_mbps;
        server_tp_sum += r.server_throughput_mbps;
    }
    
    statistics_.s1_total = s1_sum;
    statistics_.s2_total = s2_sum;
    statistics_.s3_total = s3_sum;
    statistics_.s1_avg = s1_sum / success_results.size();
    statistics_.s2_avg = s2_sum / success_results.size();
    statistics_.s3_avg = s3_sum / success_results.size();
    
    statistics_.encrypt_ratio_avg = encrypt_ratio_sum / success_results.size();
    statistics_.metadata_ratio_avg = metadata_ratio_sum / success_results.size();
    statistics_.total_overhead_avg = total_overhead_sum / success_results.size();
    
    statistics_.client_throughput_avg = client_tp_sum / success_results.size();
    statistics_.server_throughput_avg = server_tp_sum / success_results.size();
    
    // 按大小分组统计
    std::map<std::string, std::vector<FileTestResult>> groups;
    for (const auto& r : success_results) {
        std::string group = getSizeGroup(r.file_size);
        groups[group].push_back(r);
    }
    
    for (const auto& group_pair : groups) {
        const std::string& group_name = group_pair.first;
        const std::vector<FileTestResult>& group_results = group_pair.second;
        
        double t1_sum = 0, t3_sum = 0;
        for (const auto& r : group_results) {
            t1_sum += r.t1_ms;
            t3_sum += r.t3_ms;
        }
        
        statistics_.size_groups[group_name]["count"] = group_results.size();
        statistics_.size_groups[group_name]["t1_avg"] = t1_sum / group_results.size();
        statistics_.size_groups[group_name]["t3_avg"] = t3_sum / group_results.size();
    }
    
    std::cout << "[统计] ✅ 统计计算完成" << std::endl;
}

double InsertPerformanceTest::calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) return 0.0;
    
    double sum_sq_diff = 0.0;
    for (double v : values) {
        double diff = v - mean;
        sum_sq_diff += diff * diff;
    }
    
    return std::sqrt(sum_sq_diff / (values.size() - 1));
}

std::string InsertPerformanceTest::getSizeGroup(size_t size) {
    if (size < 1024) return "0-1KB";
    else if (size < 10240) return "1KB-10KB";
    else if (size < 102400) return "10KB-100KB";
    else if (size < 1048576) return "100KB-1MB";
    else return "1MB+";
}

// ==================== 报告生成 ====================

bool InsertPerformanceTest::saveDetailedReport(const std::string& csv_file) {
    std::cout << "\n[报告] 保存详细报告: " << csv_file << std::endl;
    
    std::ofstream ofs(csv_file);
    if (!ofs.is_open()) {
        std::cerr << "[错误] 无法创建CSV文件: " << csv_file << std::endl;
        return false;
    }
    
    // 写入CSV头部
    ofs << "file_id,file_path,file_size_kb,keyword_count,"
        << "t1_ms,t3_ms,"
        << "s1_bytes,s2_bytes,s3_bytes,"
        << "encrypt_ratio,metadata_ratio,total_overhead,"
        << "client_throughput_mbps,server_throughput_mbps,"
        << "timestamp,success,error_msg\n";
    
    // 写入每个结果
    int file_id = 1;
    for (const auto& r : results_) {
        ofs << file_id++ << ","
            << r.file_path << ","
            << r.file_size / 1024.0 << ","
            << r.keyword_count << ","
            << r.t1_ms << ","
            << r.t3_ms << ","
            << r.s1_bytes << ","
            << r.s2_bytes << ","
            << r.s3_bytes << ","
            << r.encrypt_ratio << ","
            << r.metadata_ratio << ","
            << r.total_overhead << ","
            << r.client_throughput_mbps << ","
            << r.server_throughput_mbps << ","
            << r.timestamp << ","
            << (r.success ? "true" : "false") << ","
            << r.error_msg << "\n";
    }
    
    ofs.close();
    std::cout << "[报告] ✅ 详细报告已保存" << std::endl;
    
    return true;
}

bool InsertPerformanceTest::saveSummaryReport(const std::string& json_file) {
    std::cout << "[报告] 保存总结报告: " << json_file << std::endl;
    
    Json::Value root;
    
    // 测试信息
    root["test_info"]["test_name"] = statistics_.test_name;
    root["test_info"]["start_time"] = statistics_.start_time;
    root["test_info"]["end_time"] = statistics_.end_time;
    root["test_info"]["total_duration_sec"] = statistics_.total_duration_sec;
    root["test_info"]["total_files"] = statistics_.total_files;
    root["test_info"]["success_count"] = statistics_.success_count;
    root["test_info"]["failure_count"] = statistics_.failure_count;
    
    // 时间统计
    root["statistics"]["time_ms"]["t1_avg"] = statistics_.t1_avg;
    root["statistics"]["time_ms"]["t1_min"] = statistics_.t1_min;
    root["statistics"]["time_ms"]["t1_max"] = statistics_.t1_max;
    root["statistics"]["time_ms"]["t1_stddev"] = statistics_.t1_stddev;
    root["statistics"]["time_ms"]["t3_avg"] = statistics_.t3_avg;
    root["statistics"]["time_ms"]["t3_min"] = statistics_.t3_min;
    root["statistics"]["time_ms"]["t3_max"] = statistics_.t3_max;
    root["statistics"]["time_ms"]["t3_stddev"] = statistics_.t3_stddev;
    
    // 数据大小统计
    root["statistics"]["size_bytes"]["s1_avg"] = (Json::Value::UInt64)statistics_.s1_avg;
    root["statistics"]["size_bytes"]["s1_total"] = (Json::Value::UInt64)statistics_.s1_total;
    root["statistics"]["size_bytes"]["s2_avg"] = (Json::Value::UInt64)statistics_.s2_avg;
    root["statistics"]["size_bytes"]["s2_total"] = (Json::Value::UInt64)statistics_.s2_total;
    root["statistics"]["size_bytes"]["s3_avg"] = (Json::Value::UInt64)statistics_.s3_avg;
    root["statistics"]["size_bytes"]["s3_total"] = (Json::Value::UInt64)statistics_.s3_total;
    
    // 比率统计
    root["statistics"]["ratios"]["encrypt_ratio_avg"] = statistics_.encrypt_ratio_avg;
    root["statistics"]["ratios"]["metadata_ratio_avg"] = statistics_.metadata_ratio_avg;
    root["statistics"]["ratios"]["total_overhead_avg"] = statistics_.total_overhead_avg;
    
    // 吞吐量统计
    root["statistics"]["throughput"]["client_mbps_avg"] = statistics_.client_throughput_avg;
    root["statistics"]["throughput"]["server_mbps_avg"] = statistics_.server_throughput_avg;
    
    // 分组统计
    for (const auto& group : statistics_.size_groups) {
        for (const auto& metric : group.second) {
            root["size_groups"][group.first][metric.first] = metric.second;
        }
    }
    
    // 写入文件
    std::ofstream ofs(json_file);
    if (!ofs.is_open()) {
        std::cerr << "[错误] 无法创建JSON文件: " << json_file << std::endl;
        return false;
    }
    
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    ofs << Json::writeString(writer, root);
    ofs.close();
    
    std::cout << "[报告] ✅ 总结报告已保存" << std::endl;
    
    return true;
}

// ==================== 辅助函数 ====================

void InsertPerformanceTest::printProgress(int current, int total) {
    int bar_width = 50;
    float progress = (float)current / total;
    int pos = bar_width * progress;
    
    std::cout << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% (" << current << "/" << total << ")\r";
    std::cout.flush();
    
    if (current == total) {
        std::cout << std::endl;
    }
}

void InsertPerformanceTest::printSummary() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "测试总结" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    std::cout << "\n📊 基本信息:" << std::endl;
    std::cout << "  测试名称: " << statistics_.test_name << std::endl;
    std::cout << "  开始时间: " << statistics_.start_time << std::endl;
    std::cout << "  结束时间: " << statistics_.end_time << std::endl;
    std::cout << "  总耗时: " << statistics_.total_duration_sec << " 秒" << std::endl;
    std::cout << "  总文件数: " << statistics_.total_files << std::endl;
    std::cout << "  成功: " << statistics_.success_count << " / 失败: " << statistics_.failure_count << std::endl;
    
    std::cout << "\n⏱️  时间统计 (毫秒):" << std::endl;
    std::cout << "  T1 (客户端加密):" << std::endl;
    std::cout << "    平均: " << statistics_.t1_avg << " ms" << std::endl;
    std::cout << "    最小: " << statistics_.t1_min << " ms" << std::endl;
    std::cout << "    最大: " << statistics_.t1_max << " ms" << std::endl;
    std::cout << "    标准差: " << statistics_.t1_stddev << " ms" << std::endl;
    
    std::cout << "  T3 (服务端插入):" << std::endl;
    std::cout << "    平均: " << statistics_.t3_avg << " ms" << std::endl;
    std::cout << "    最小: " << statistics_.t3_min << " ms" << std::endl;
    std::cout << "    最大: " << statistics_.t3_max << " ms" << std::endl;
    std::cout << "    标准差: " << statistics_.t3_stddev << " ms" << std::endl;
    
    std::cout << "\n💾 数据大小统计:" << std::endl;
    std::cout << "  S1 (明文): 平均 " << statistics_.s1_avg << " bytes, 总计 " << statistics_.s1_total << " bytes" << std::endl;
    std::cout << "  S2 (密文): 平均 " << statistics_.s2_avg << " bytes, 总计 " << statistics_.s2_total << " bytes" << std::endl;
    std::cout << "  S3 (JSON): 平均 " << statistics_.s3_avg << " bytes, 总计 " << statistics_.s3_total << " bytes" << std::endl;
    
    std::cout << "\n📈 比率统计 (%):" << std::endl;
    std::cout << "  加密膨胀率: " << statistics_.encrypt_ratio_avg << "%" << std::endl;
    std::cout << "  元数据占比: " << statistics_.metadata_ratio_avg << "%" << std::endl;
    std::cout << "  总开销: " << statistics_.total_overhead_avg << "%" << std::endl;
    
    std::cout << "\n🚀 吞吐量 (MB/s):" << std::endl;
    std::cout << "  客户端: " << statistics_.client_throughput_avg << " MB/s" << std::endl;
    std::cout << "  服务端: " << statistics_.server_throughput_avg << " MB/s" << std::endl;
    
    std::cout << "\n📦 按文件大小分组:" << std::endl;
    for (const auto& group : statistics_.size_groups) {
        std::cout << "  " << group.first << ": "
                  << "数量=" << (int)group.second.at("count")
                  << ", T1平均=" << group.second.at("t1_avg") << "ms"
                  << ", T3平均=" << group.second.at("t3_avg") << "ms" << std::endl;
    }
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
}

std::string InsertPerformanceTest::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void InsertPerformanceTest::clearPerformanceData() {
    current_times_.clear();
    current_sizes_.clear();
}

std::string InsertPerformanceTest::resolveFilePath(const std::string& raw_path) const {
    fs::path original(raw_path);
    if (fs::exists(original)) {
        return original.lexically_normal().string();
    }
    
    if (!base_dir_.empty()) {
        fs::path base(base_dir_);
        std::string raw_str = raw_path;
        std::string base_name = base.filename().string();
        
        auto pos = raw_str.find(base_name);
        if (pos != std::string::npos) {
            std::string relative_tail = raw_str.substr(pos + base_name.length());
            if (!relative_tail.empty() && (relative_tail[0] == '/' || relative_tail[0] == '\\')) {
                relative_tail = relative_tail.substr(1);
            }
            
            fs::path candidate = base / relative_tail;
            if (fs::exists(candidate)) {
                return candidate.lexically_normal().string();
            }
        }
        
        fs::path filename_only = base / original.filename();
        if (fs::exists(filename_only)) {
            return filename_only.lexically_normal().string();
        }
    }
    
    return original.lexically_normal().string();
}

std::string InsertPerformanceTest::makeSafeName(const std::string& file_path) const {
    fs::path abs_path = fs::absolute(file_path).lexically_normal();
    std::string abs_str = abs_path.string();
    std::string safe = abs_str;
    std::replace(safe.begin(), safe.end(), '/', '_');
    std::replace(safe.begin(), safe.end(), '\\', '_');
    std::replace(safe.begin(), safe.end(), ':', '_');
    return safe;
}

// ==================== MAIN函数 ====================

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║          插入操作性能测试程序                                  ║
║          Insert Performance Test                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;
    
    // 获取配置文件路径
    std::string config_file = kDefaultConfigPath;
    
    // 创建测试实例
    InsertPerformanceTest test;
    
    // 1. 加载配置
    std::cout << "步骤 1/4: 加载配置..." << std::endl;
    if (!test.loadConfig(config_file)) {
        std::cerr << "\n❌ 配置加载失败" << std::endl;
        return 1;
    }
    
    // 2. 初始化
    std::cout << "\n步骤 2/4: 初始化环境..." << std::endl;
    if (!test.initialize()) {
        std::cerr << "\n❌ 初始化失败" << std::endl;
        return 1;
    }
    
    // 3. 运行测试
    std::cout << "\n步骤 3/4: 运行测试..." << std::endl;
    if (!test.runTest()) {
        std::cerr << "\n❌ 测试执行失败" << std::endl;
        return 1;
    }
    
    // 4. 保存结果
    std::cout << "\n步骤 4/4: 保存结果..." << std::endl;
    test.saveDetailedReport("results/insert_detailed.csv");
    test.saveSummaryReport("results/insert_summary.json");
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "✅ 测试完成！" << std::endl;
    std::cout << "详细报告: results/insert_detailed.csv" << std::endl;
    std::cout << "总结报告: results/insert_summary.json" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    return 0;
}
