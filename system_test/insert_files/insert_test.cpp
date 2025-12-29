#include "./insert_test.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <numeric>

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
    
    // 提取配置
    keywords_file_ = config["data_source"]["keywords_file"].asString();
    base_dir_ = config["data_source"]["base_dir"].asString();
    
    public_params_file_ = config["client_config"]["public_params"].asString();
    client_data_dir_ = config["client_config"]["data_dir"].asString();
    
    server_data_dir_ = config["server_config"]["data_dir"].asString();
    server_port_ = config["server_config"]["port"].asInt();
    
    max_files_ = config["options"]["max_files"].asInt();
    verbose_ = config["options"]["verbose"].asBool();
    save_intermediate_ = config["options"]["save_intermediate"].asBool();
    
    statistics_.test_name = config["test_name"].asString();
    
    std::cout << "[配置] 关键词文件: " << keywords_file_ << std::endl;
    std::cout << "[配置] 数据目录: " << base_dir_ << std::endl;
    std::cout << "[配置] 最大文件数: " << (max_files_ > 0 ? std::to_string(max_files_) : "全部") << std::endl;
    
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
    if (!files.isArray()) {
        std::cerr << "[错误] 'files'字段不是数组" << std::endl;
        return false;
    }
    
    for (const auto& file_entry : files) {
        std::string path = file_entry["path"].asString();
        std::vector<std::string> keywords;
        
        const Json::Value& kw_array = file_entry["keywords"];
        for (const auto& kw : kw_array) {
            keywords.push_back(kw.asString());
        }
        
        file_keywords_map_[path] = keywords;
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
    
    if (!client_->initialize(public_params_file_)) {
        std::cerr << "[错误] 客户端初始化失败" << std::endl;
        return false;
    }
    
    if (!client_->initializeDataDirectories()) {
        std::cerr << "[错误] 客户端目录初始化失败" << std::endl;
        return false;
    }
    
    // 加载或生成密钥
    if (!client_->loadKeys("private_key.dat")) {
        std::cout << "[初始化] 未找到密钥，生成新密钥..." << std::endl;
        if (!client_->generateKeys()) {
            std::cerr << "[错误] 密钥生成失败" << std::endl;
            return false;
        }
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
    
    std::cout << "[初始化] ✅ 初始化完成" << std::endl;
    
    return true;
}

// ==================== 测试执行 ====================

bool InsertPerformanceTest::runTest() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "开始插入性能测试" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
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
        
        // 找到生成的insert.json和.enc文件
        // 这里简化处理，实际应该从client的输出中获取
        std::string filename = file_path.substr(file_path.find_last_of("/\\") + 1);
        std::string enc_file = client_data_dir_ + "/EncFiles/" + filename + ".enc";
        std::string insert_json = client_data_dir_ + "/Insert/" + filename + "_insert.json";
        
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

// ==================== MAIN函数 ====================

int main(int argc, char* argv[]) {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║          插入操作性能测试程序                                  ║
║          Insert Performance Test                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;
    
    // 获取配置文件路径
    std::string config_file = "test_config.json";
    if (argc > 1) {
        config_file = argv[1];
    }
    
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
    test.saveDetailedReport("insert_performance_report.csv");
    test.saveSummaryReport("insert_performance_summary.json");
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "✅ 测试完成！" << std::endl;
    std::cout << "详细报告: insert_performance_report.csv" << std::endl;
    std::cout << "总结报告: insert_performance_summary.json" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    return 0;
}