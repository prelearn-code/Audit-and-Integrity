#include "./search_test.h"
#include <numeric>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace fs = std::filesystem;

namespace {
const char* kDefaultConfig = "config/search_test_config.json";
}

SearchPerformanceTest::SearchPerformanceTest()
    : client_(nullptr), server_(nullptr),
      server_port_(9000), max_keywords_(0),
      verbose_(true), save_intermediate_(true),
      use_keyword_states_(false), verify_proof_(false) {
}

SearchPerformanceTest::~SearchPerformanceTest() {
    delete client_;
    delete server_;
}

bool SearchPerformanceTest::loadConfig(const std::string& config_file) {
    std::cout << "\n[配置] 加载搜索测试配置: " << config_file << std::endl;
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

    const Json::Value& paths = config["paths"];
    keywords_file_ = fs::path(paths.get("keywords_file", "").asString()).lexically_normal().string();
    public_params_file_ = fs::path(paths.get("public_params", "").asString()).lexically_normal().string();
    private_key_file_ = fs::path(paths.get("private_key", "private_key.dat").asString()).lexically_normal().string();

    const Json::Value& client_cfg = paths["client"];
    client_data_dir_ = fs::path(client_cfg.get("data_dir", "../../vds-client/data").asString()).lexically_normal().string();
    client_insert_dir_ = fs::path(client_cfg.get("insert_dir", client_data_dir_ + "/Insert").asString()).lexically_normal().string();
    client_enc_dir_ = fs::path(client_cfg.get("enc_dir", client_data_dir_ + "/EncFiles").asString()).lexically_normal().string();
    client_meta_dir_ = fs::path(client_cfg.get("metadata_dir", client_data_dir_ + "/MetaFiles").asString()).lexically_normal().string();
    client_search_dir_ = fs::path(client_cfg.get("search_dir", client_data_dir_ + "/Search").asString()).lexically_normal().string();
    client_deles_dir_ = fs::path(client_cfg.get("deles_dir", client_data_dir_ + "/Deles").asString()).lexically_normal().string();
    keyword_states_file_ = fs::path(client_cfg.get("keyword_states_file", client_data_dir_ + "/keyword_states.json").asString()).lexically_normal().string();

    const Json::Value& server_cfg = paths["server"];
    server_data_dir_ = fs::path(server_cfg.get("data_dir", "../../Storage-node/data").asString()).lexically_normal().string();
    server_search_proof_dir_ = fs::path(server_cfg.get("search_proof_dir", server_data_dir_ + "/SearchProof").asString()).lexically_normal().string();
    server_port_ = server_cfg.get("port", 9000).asInt();

    const Json::Value& options = config["options"];
    max_keywords_ = options.get("max_keywords", 0).asInt();
    verbose_ = options.get("verbose", true).asBool();
    save_intermediate_ = options.get("save_intermediate", true).asBool();
    use_keyword_states_ = options.get("use_keyword_states", false).asBool();
    verify_proof_ = options.get("verify_proof", false).asBool();

    statistics_.test_name = config.get("test_name", "search_performance").asString();

    if (!fs::exists(keywords_file_)) {
        std::cerr << "[错误] 关键词文件不存在: " << keywords_file_ << std::endl;
        return false;
    }

    std::cout << "[配置] 关键词文件: " << keywords_file_ << std::endl;
    std::cout << "[配置] 客户端搜索目录: " << client_search_dir_ << std::endl;
    std::cout << "[配置] 服务端搜索目录: " << server_search_proof_dir_ << std::endl;
    std::cout << "[配置] 使用keyword_states: " << (use_keyword_states_ ? "是" : "否") << std::endl;
    return true;
}

bool SearchPerformanceTest::loadKeywords() {
    if (use_keyword_states_) {
        Json::Value root;
        if (!readJson(keyword_states_file_, root)) {
            std::cerr << "[错误] 读取 keyword_states.json 失败: " << keyword_states_file_ << std::endl;
            return false;
        }
        const Json::Value& kw_obj = root["keywords"];
        if (!kw_obj.isObject()) {
            std::cerr << "[错误] keyword_states.json 缺少 keywords 对象" << std::endl;
            return false;
        }
        for (const auto& name : kw_obj.getMemberNames()) {
            keywords_.push_back(name);
        }
    } else {
        Json::Value root;
        if (!readJson(keywords_file_, root)) {
            return false;
        }
        const Json::Value& arr = root["keywords"];
        if (!arr.isArray()) {
            std::cerr << "[错误] keywords字段不是数组" << std::endl;
            return false;
        }
        for (const auto& v : arr) {
            keywords_.push_back(v.asString());
        }
    }

    if (max_keywords_ > 0 && keywords_.size() > (size_t)max_keywords_) {
        keywords_.resize(max_keywords_);
    }

    std::cout << "[数据] 已加载关键词数量: " << keywords_.size() << std::endl;
    return true;
}

bool SearchPerformanceTest::initialize() {
    if (!loadKeywords()) return false;

    // 初始化客户端
    client_ = new StorageClient();
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
    if (!client_->loadKeys(private_key_file_)) {
        std::cout << "[初始化] 未找到密钥，生成新密钥..." << std::endl;
        if (!client_->generateKeys(private_key_file_)) {
            std::cerr << "[错误] 密钥生成失败" << std::endl;
            return false;
        }
    }

    std::cout << "[初始化] 客户端初始化完成" << std::endl;

    // 初始化服务端（提前加载数据库和索引）
    server_ = new StorageNode(server_data_dir_, server_port_);

    // 预加载服务端的数据库和索引 - 这部分时间不计入性能测试
    std::cout << "[初始化] 服务端预加载数据库和索引..." << std::endl;
    auto load_start = std::chrono::high_resolution_clock::now();

    if (!server_->load_index_database()) {
        std::cerr << "[错误] 服务端加载索引数据库失败" << std::endl;
        return false;
    }
    if (!server_->load_search_database()) {
        std::cerr << "[错误] 服务端加载搜索数据库失败" << std::endl;
        return false;
    }

    auto load_end = std::chrono::high_resolution_clock::now();
    double load_time_ms = std::chrono::duration<double, std::milli>(load_end - load_start).count();

    std::cout << "[初始化] 服务端数据加载完成 (耗时: " << std::fixed << std::setprecision(2)
              << load_time_ms << " ms，不计入性能测试)" << std::endl;
    std::cout << "[初始化] 索引条目数: " << server_->index_database.size() << std::endl;
    std::cout << "[初始化] 搜索索引条目数: " << server_->search_database.size() << std::endl;

    return true;
}

SearchPerformanceTest::KeywordTestResult SearchPerformanceTest::testSingleKeyword(const std::string& keyword) {
    KeywordTestResult result;
    result.keyword = keyword;
    result.timestamp = getCurrentTimestamp();
    result.success = false;

    // ==================== 客户端：生成搜索Token ====================
    if (verbose_) {
        std::cout << "\n[测试] 关键词: " << keyword << std::endl;
        std::cout << "  [客户端] 生成搜索Token..." << std::endl;
    }

    // 精确测量Token生成时间
    auto client_start = std::chrono::high_resolution_clock::now();
    bool client_success = client_->searchKeyword(keyword);
    auto client_end = std::chrono::high_resolution_clock::now();

    result.t_client_token_gen_ms = std::chrono::duration<double, std::milli>(client_end - client_start).count();

    if (!client_success) {
        result.error_msg = "Token生成失败";
        if (verbose_) {
            std::cout << "  ❌ " << result.error_msg << std::endl;
        }
        return result;
    }

    // 获取Token文件路径和大小
    std::string token_file = client_search_dir_ + "/" + keyword + ".json";
    if (fs::exists(token_file)) {
        result.token_size_bytes = fs::file_size(token_file);
    }

    if (verbose_) {
        std::cout << "  ✅ Token生成完成 (" << std::fixed << std::setprecision(3)
                  << result.t_client_token_gen_ms << " ms)" << std::endl;
        std::cout << "  📄 Token大小: " << result.token_size_bytes << " bytes" << std::endl;
    }

    // 读取Token以获取T值（用于后续读取证明文件）
    Json::Value search_params;
    std::string token_value;
    if (readJson(token_file, search_params)) {
        token_value = search_params.get("T", "").asString();
    }

    // ==================== 服务端：纯证明计算（不含加载） ====================
    if (verbose_) {
        std::cout << "  [服务端] 计算搜索证明..." << std::endl;
    }

    // 精确测量证明计算时间（数据库已经预加载，只测量证明计算）
    auto server_start = std::chrono::high_resolution_clock::now();
    bool server_success = server_->SearchKeywordsAssociatedFilesProof(token_file);
    auto server_end = std::chrono::high_resolution_clock::now();

    result.t_server_proof_calc_ms = std::chrono::duration<double, std::milli>(server_end - server_start).count();

    if (!server_success) {
        result.error_msg = "证明计算失败";
        if (verbose_) {
            std::cout << "  ❌ " << result.error_msg << std::endl;
        }
        return result;
    }

    // 获取证明文件路径和大小（使用token值作为文件名）
    std::string proof_file;
    if (!token_value.empty()) {
        proof_file = server_search_proof_dir_ + "/" + token_value + ".json";
    }

    if (!proof_file.empty() && fs::exists(proof_file)) {
        result.proof_size_bytes = fs::file_size(proof_file);

        // 读取证明JSON获取结果数量
        Json::Value proof_json;
        if (readJson(proof_file, proof_json)) {
            // 尝试新格式 (file_proofs)
            if (proof_json.isMember("file_proofs") && proof_json["file_proofs"].isArray()) {
                result.result_count = proof_json["file_proofs"].size();
            }
            // 尝试旧格式 (AS)
            else if (proof_json.isMember("AS") && proof_json["AS"].isArray()) {
                result.result_count = proof_json["AS"].size();
            }
        }
    }

    if (verbose_) {
        std::cout << "  ✅ 证明计算完成 (" << std::fixed << std::setprecision(3)
                  << result.t_server_proof_calc_ms << " ms)" << std::endl;
        std::cout << "  📄 证明大小: " << result.proof_size_bytes << " bytes" << std::endl;
        std::cout << "  🔍 命中文件数: " << result.result_count << std::endl;
    }

    result.success = true;
    return result;
}

bool SearchPerformanceTest::cleanupData() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🧹 清理搜索测试产生的数据" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    namespace fs = std::filesystem;

    // 清理客户端搜索Token文件
    std::cout << "[清理] 清理客户端搜索数据..." << std::endl;
    if (fs::exists(client_search_dir_)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(client_search_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                // 删除所有JSON文件（都是搜索token）
                fs::remove(entry.path());
                count++;
            }
        }
        std::cout << "  ✅ 删除搜索Token文件: " << count << " 个" << std::endl;
    }

    // 清理服务端搜索证明文件
    std::cout << "[清理] 清理服务端搜索证明数据..." << std::endl;
    if (fs::exists(server_search_proof_dir_)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(server_search_proof_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                // 删除所有JSON文件（都是证明文件）
                fs::remove(entry.path());
                count++;
            }
        }
        std::cout << "  ✅ 删除搜索证明文件: " << count << " 个" << std::endl;
    }

    std::cout << "\n✅ 搜索数据清理完成\n" << std::endl;
    return true;
}

bool SearchPerformanceTest::runTest() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "开始搜索性能测试" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    // 清理之前的搜索数据
    if (!cleanupData()) {
        std::cerr << "❌ 搜索数据清理失败" << std::endl;
        return false;
    }

    statistics_.start_time = getCurrentTimestamp();
    auto test_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < keywords_.size(); i++) {
        std::cout << "\n进度: [" << (i + 1) << "/" << keywords_.size() << "]" << std::endl;

        KeywordTestResult result = testSingleKeyword(keywords_[i]);
        results_.push_back(result);

        if (result.success) {
            statistics_.success_count++;
        } else {
            statistics_.failure_count++;
        }
    }

    auto test_end = std::chrono::high_resolution_clock::now();
    statistics_.end_time = getCurrentTimestamp();
    statistics_.total_duration_sec = std::chrono::duration<double>(test_end - test_start).count();
    statistics_.total_keywords = keywords_.size();

    calculateStatistics();

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "测试完成" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    printSummary();

    return true;
}

void SearchPerformanceTest::calculateStatistics() {
    if (results_.empty()) return;

    std::vector<double> client_times;
    std::vector<double> server_times;
    std::vector<size_t> token_sizes;
    std::vector<size_t> proof_sizes;

    for (const auto& r : results_) {
        if (r.success) {
            client_times.push_back(r.t_client_token_gen_ms);
            server_times.push_back(r.t_server_proof_calc_ms);
            token_sizes.push_back(r.token_size_bytes);
            proof_sizes.push_back(r.proof_size_bytes);
        }
    }

    if (!client_times.empty()) {
        // 客户端统计
        statistics_.total_client_time_ms = std::accumulate(client_times.begin(), client_times.end(), 0.0);
        statistics_.client_token_avg_ms = statistics_.total_client_time_ms / client_times.size();
        statistics_.client_token_min_ms = *std::min_element(client_times.begin(), client_times.end());
        statistics_.client_token_max_ms = *std::max_element(client_times.begin(), client_times.end());
        statistics_.client_token_stddev_ms = calculateStdDev(client_times, statistics_.client_token_avg_ms);
        statistics_.client_qps = (statistics_.total_client_time_ms > 0) ?
            (client_times.size() * 1000.0 / statistics_.total_client_time_ms) : 0.0;

        // 服务端统计
        statistics_.total_server_time_ms = std::accumulate(server_times.begin(), server_times.end(), 0.0);
        statistics_.server_proof_avg_ms = statistics_.total_server_time_ms / server_times.size();
        statistics_.server_proof_min_ms = *std::min_element(server_times.begin(), server_times.end());
        statistics_.server_proof_max_ms = *std::max_element(server_times.begin(), server_times.end());
        statistics_.server_proof_stddev_ms = calculateStdDev(server_times, statistics_.server_proof_avg_ms);
        statistics_.server_qps = (statistics_.total_server_time_ms > 0) ?
            (server_times.size() * 1000.0 / statistics_.total_server_time_ms) : 0.0;

        // 数据大小统计
        statistics_.token_avg_bytes = std::accumulate(token_sizes.begin(), token_sizes.end(), 0UL) / token_sizes.size();
        statistics_.proof_avg_bytes = std::accumulate(proof_sizes.begin(), proof_sizes.end(), 0UL) / proof_sizes.size();
    }
}

double SearchPerformanceTest::calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) return 0.0;

    double sum_sq_diff = 0.0;
    for (double v : values) {
        double diff = v - mean;
        sum_sq_diff += diff * diff;
    }
    return std::sqrt(sum_sq_diff / (values.size() - 1));
}

void SearchPerformanceTest::printSummary() {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📊 性能测试总结" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    std::cout << "测试名称: " << statistics_.test_name << std::endl;
    std::cout << "开始时间: " << statistics_.start_time << std::endl;
    std::cout << "结束时间: " << statistics_.end_time << std::endl;
    std::cout << "总耗时: " << std::fixed << std::setprecision(2)
              << statistics_.total_duration_sec << " 秒" << std::endl;
    std::cout << "总关键词数: " << statistics_.total_keywords << std::endl;
    std::cout << "成功: " << statistics_.success_count << " | 失败: " << statistics_.failure_count << std::endl;

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "💻 客户端性能（Token生成）" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "总时间: " << std::fixed << std::setprecision(2)
              << statistics_.total_client_time_ms << " ms" << std::endl;
    std::cout << "平均时间: " << std::fixed << std::setprecision(3)
              << statistics_.client_token_avg_ms << " ms" << std::endl;
    std::cout << "最小时间: " << std::fixed << std::setprecision(3)
              << statistics_.client_token_min_ms << " ms" << std::endl;
    std::cout << "最大时间: " << std::fixed << std::setprecision(3)
              << statistics_.client_token_max_ms << " ms" << std::endl;
    std::cout << "标准差: " << std::fixed << std::setprecision(3)
              << statistics_.client_token_stddev_ms << " ms" << std::endl;
    std::cout << "QPS: " << std::fixed << std::setprecision(2)
              << statistics_.client_qps << " 查询/秒" << std::endl;

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔧 服务端性能（纯证明计算，不含加载）" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "总时间: " << std::fixed << std::setprecision(2)
              << statistics_.total_server_time_ms << " ms" << std::endl;
    std::cout << "平均时间: " << std::fixed << std::setprecision(3)
              << statistics_.server_proof_avg_ms << " ms" << std::endl;
    std::cout << "最小时间: " << std::fixed << std::setprecision(3)
              << statistics_.server_proof_min_ms << " ms" << std::endl;
    std::cout << "最大时间: " << std::fixed << std::setprecision(3)
              << statistics_.server_proof_max_ms << " ms" << std::endl;
    std::cout << "标准差: " << std::fixed << std::setprecision(3)
              << statistics_.server_proof_stddev_ms << " ms" << std::endl;
    std::cout << "QPS: " << std::fixed << std::setprecision(2)
              << statistics_.server_qps << " 查询/秒" << std::endl;

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📦 数据大小统计" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "平均Token大小: " << statistics_.token_avg_bytes << " bytes" << std::endl;
    std::cout << "平均证明大小: " << statistics_.proof_avg_bytes << " bytes" << std::endl;
    std::cout << std::endl;
}

bool SearchPerformanceTest::saveDetailedReport(const std::string& csv_file) {
    std::cout << "[报告] 保存详细报告: " << csv_file << std::endl;

    // 确保目录存在
    fs::path csv_path(csv_file);
    if (csv_path.has_parent_path()) {
        fs::create_directories(csv_path.parent_path());
    }

    std::ofstream ofs(csv_file);
    if (!ofs.is_open()) {
        std::cerr << "[错误] 无法创建CSV文件: " << csv_file << std::endl;
        return false;
    }

    // CSV头
    ofs << "keyword,"
        << "client_token_gen_ms,token_size_bytes,"
        << "server_proof_calc_ms,proof_size_bytes,result_count,"
        << "timestamp,success,error_msg\n";

    // 数据行
    for (const auto& r : results_) {
        ofs << r.keyword << ","
            << std::fixed << std::setprecision(6) << r.t_client_token_gen_ms << ","
            << r.token_size_bytes << ","
            << std::fixed << std::setprecision(6) << r.t_server_proof_calc_ms << ","
            << r.proof_size_bytes << ","
            << r.result_count << ","
            << r.timestamp << ","
            << (r.success ? "true" : "false") << ","
            << r.error_msg << "\n";
    }

    ofs.close();
    std::cout << "[报告] ✅ 详细报告已保存" << std::endl;
    return true;
}

bool SearchPerformanceTest::saveSummaryReport(const std::string& json_file) {
    std::cout << "[报告] 保存总结报告: " << json_file << std::endl;

    // 确保目录存在
    fs::path json_path(json_file);
    if (json_path.has_parent_path()) {
        fs::create_directories(json_path.parent_path());
    }

    Json::Value root;

    // 测试信息
    root["test_info"]["test_name"] = statistics_.test_name;
    root["test_info"]["start_time"] = statistics_.start_time;
    root["test_info"]["end_time"] = statistics_.end_time;
    root["test_info"]["total_duration_sec"] = statistics_.total_duration_sec;
    root["test_info"]["total_keywords"] = statistics_.total_keywords;
    root["test_info"]["success_count"] = statistics_.success_count;
    root["test_info"]["failure_count"] = statistics_.failure_count;

    // 客户端统计
    root["client_performance"]["total_time_ms"] = statistics_.total_client_time_ms;
    root["client_performance"]["token_gen_avg_ms"] = statistics_.client_token_avg_ms;
    root["client_performance"]["token_gen_min_ms"] = statistics_.client_token_min_ms;
    root["client_performance"]["token_gen_max_ms"] = statistics_.client_token_max_ms;
    root["client_performance"]["token_gen_stddev_ms"] = statistics_.client_token_stddev_ms;
    root["client_performance"]["qps"] = statistics_.client_qps;

    // 服务端统计
    root["server_performance"]["total_time_ms"] = statistics_.total_server_time_ms;
    root["server_performance"]["proof_calc_avg_ms"] = statistics_.server_proof_avg_ms;
    root["server_performance"]["proof_calc_min_ms"] = statistics_.server_proof_min_ms;
    root["server_performance"]["proof_calc_max_ms"] = statistics_.server_proof_max_ms;
    root["server_performance"]["proof_calc_stddev_ms"] = statistics_.server_proof_stddev_ms;
    root["server_performance"]["qps"] = statistics_.server_qps;
    root["server_performance"]["note"] = "Pure proof calculation time, excluding database loading";

    // 数据大小
    root["data_size"]["token_avg_bytes"] = (Json::Value::UInt64)statistics_.token_avg_bytes;
    root["data_size"]["proof_avg_bytes"] = (Json::Value::UInt64)statistics_.proof_avg_bytes;

    std::ofstream ofs(json_file);
    if (!ofs.is_open()) {
        std::cerr << "[错误] 无法创建JSON文件: " << json_file << std::endl;
        return false;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &ofs);
    ofs << std::endl;

    ofs.close();
    std::cout << "[报告] ✅ 总结报告已保存" << std::endl;
    return true;
}

std::string SearchPerformanceTest::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool SearchPerformanceTest::readJson(const std::string& path, Json::Value& out) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "[错误] 无法打开JSON: " << path << std::endl;
        return false;
    }
    Json::CharReaderBuilder reader;
    std::string errs;
    if (!Json::parseFromStream(reader, ifs, &out, &errs)) {
        std::cerr << "[错误] JSON解析失败: " << errs << std::endl;
        return false;
    }
    return true;
}

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════╗
║          搜索性能测试程序 v2.0                      ║
║          Search Performance Test                  ║
║                                                    ║
║  客户端: Token生成时间                              ║
║  服务端: 纯证明计算时间(不含加载)                    ║
╚══════════════════════════════════════════════════╝
)" << std::endl;

    SearchPerformanceTest test;
    if (!test.loadConfig(kDefaultConfig)) {
        return 1;
    }
    if (!test.initialize()) {
        return 1;
    }
    if (!test.runTest()) {
        return 1;
    }
    test.saveDetailedReport("results/search_detailed.csv");
    test.saveSummaryReport("results/search_summary.json");

    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "✅ 测试完成！" << std::endl;
    std::cout << "详细报告: results/search_detailed.csv" << std::endl;
    std::cout << "总结报告: results/search_summary.json" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    return 0;
}
