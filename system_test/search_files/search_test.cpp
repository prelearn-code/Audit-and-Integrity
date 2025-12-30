#include "./search_test.h"
#include <numeric>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

namespace {
const char* kDefaultConfig = "config/search_test_config.json";
}

SearchPerformanceTest::SearchPerformanceTest()
    : client_(nullptr), server_(nullptr), server_port_(9000), max_keywords_(0), verbose_(true), save_intermediate_(true) {
    callback_c_.on_phase_complete = [this](const std::string& name, double time_ms) {
        current_times_[name] = time_ms;
        if (verbose_) {
            std::cout << "  [TIME] " << name << ": " << time_ms << " ms" << std::endl;
        }
    };
    callback_c_.on_data_size_recorded = [this](const std::string& name, size_t size_bytes) {
        current_sizes_[name] = size_bytes;
        if (verbose_) {
            std::cout << "  [SIZE] " << name << ": " << size_bytes << " bytes" << std::endl;
        }
    };
    callback_s_.on_phase_complete = [this](const std::string& name, double time_ms) {
        current_times_[name] = time_ms;
        if (verbose_) {
            std::cout << "  [TIME] " << name << ": " << time_ms << " ms" << std::endl;
        }
    };
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
    client_data_dir_ = fs::path(client_cfg.get("data_dir", "vds-client/data").asString()).lexically_normal().string();
    client_insert_dir_ = fs::path(client_cfg.get("insert_dir", client_data_dir_ + "/Insert").asString()).lexically_normal().string();
    client_enc_dir_ = fs::path(client_cfg.get("enc_dir", client_data_dir_ + "/EncFiles").asString()).lexically_normal().string();
    client_meta_dir_ = fs::path(client_cfg.get("metadata_dir", client_data_dir_ + "/MetaFiles").asString()).lexically_normal().string();
    client_search_dir_ = fs::path(client_cfg.get("search_dir", client_data_dir_ + "/Search").asString()).lexically_normal().string();
    client_deles_dir_ = fs::path(client_cfg.get("deles_dir", client_data_dir_ + "/Deles").asString()).lexically_normal().string();
    keyword_states_file_ = fs::path(client_cfg.get("keyword_states_file", client_data_dir_ + "/keyword_states.json").asString()).lexically_normal().string();

    const Json::Value& server_cfg = paths["server"];
    server_data_dir_ = fs::path(server_cfg.get("data_dir", "Storage-node/data").asString()).lexically_normal().string();
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
    std::cout << "[数据] 已加载关键词数量: " << keywords_.size() << std::endl;
    return true;
}

bool SearchPerformanceTest::initialize() {
    if (!loadKeywords()) return false;

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
        client_->saveKeys(private_key_file_);
    }
    client_->setPerformanceCallback_c(&callback_c_);

    server_ = new StorageNode(server_data_dir_, server_port_);
    if (!server_->load_public_params(public_params_file_)) {
        std::cerr << "[错误] 服务端加载公共参数失败" << std::endl;
        return false;
    }
    if (!server_->initialize_directories()) {
        std::cerr << "[错误] 服务端目录初始化失败" << std::endl;
        return false;
    }
    server_->load_index_database();
    server_->load_search_database();
    server_->setPerformanceCallback_s(&callback_s_);

    return true;
}

SearchPerformanceTest::KeywordTestResult SearchPerformanceTest::testSingleKeyword(const std::string& keyword) {
    KeywordTestResult result{};
    result.keyword = keyword;
    result.timestamp = getCurrentTimestamp();
    result.success = false;

    clearPerformanceData();

    std::string search_json = client_search_dir_ + "/" + keyword + ".json";
    Json::Value search_params;

    // 客户端：生成搜索令牌
    auto t_client_start = std::chrono::high_resolution_clock::now();
    if (!client_->searchKeyword(keyword)) {
        result.error_msg = "客户端生成搜索令牌失败";
        return result;
    }
    auto t_client_end = std::chrono::high_resolution_clock::now();
    result.t_client_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_client_end - t_client_start).count();
    if (current_times_.count("token_generation")) {
        result.t_client_ms = current_times_["token_generation"];
    }
    result.request_size = current_sizes_["search_request_size"];

    if (!readJson(search_json, search_params)) {
        result.error_msg = "读取搜索请求失败";
        return result;
    }
    std::string token = search_params.get("T", "").asString();
    if (token.empty()) {
        result.error_msg = "搜索请求缺少令牌";
        return result;
    }

    // 服务端：执行搜索证明
    clearPerformanceData();
    auto t_server_start = std::chrono::high_resolution_clock::now();
    if (!server_->SearchKeywordsAssociatedFilesProof(search_json)) {
        result.error_msg = "服务端搜索失败";
        return result;
    }
    auto t_server_end = std::chrono::high_resolution_clock::now();
    result.t_server_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_server_end - t_server_start).count();
    if (current_times_.count("server_search_total")) {
        result.t_server_ms = current_times_["server_search_total"];
    }

    // 读取证明文件大小与命中数
    fs::path proof_path = fs::path(server_data_dir_) / "SearchProof" / (token + ".json");
    if (fs::exists(proof_path)) {
        result.proof_size = fs::file_size(proof_path);
        Json::Value proof_json;
        if (readJson(proof_path.string(), proof_json)) {
            const Json::Value& AS = proof_json["AS"];
            if (AS.isArray()) {
                result.result_count = AS.size();
            }
        }
        if (verify_proof_) {
            clearPerformanceData();
            if (!server_->VerifySearchProof(proof_path.string())) {
                result.error_msg = "搜索证明验证失败";
                result.success = false;
                return result;
            }
        }
    }

    result.success = true;
    return result;
}

void SearchPerformanceTest::calculateStatistics() {
    std::vector<KeywordTestResult> success;
    for (const auto& r : results_) {
        if (r.success) success.push_back(r);
    }
    statistics_.total_keywords = results_.size();
    statistics_.success_count = success.size();
    statistics_.failure_count = results_.size() - success.size();
    if (success.empty()) return;

    auto avg = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };

    std::vector<double> tc, ts;
    std::vector<size_t> req, proof;
    for (const auto& r : success) {
        tc.push_back(r.t_client_ms);
        ts.push_back(r.t_server_ms);
        req.push_back(r.request_size);
        proof.push_back(r.proof_size);
    }
    statistics_.t_client_avg = avg(tc);
    statistics_.t_server_avg = avg(ts);
    statistics_.t_client_min = *std::min_element(tc.begin(), tc.end());
    statistics_.t_client_max = *std::max_element(tc.begin(), tc.end());
    statistics_.t_server_min = *std::min_element(ts.begin(), ts.end());
    statistics_.t_server_max = *std::max_element(ts.begin(), ts.end());
    statistics_.request_avg = std::accumulate(req.begin(), req.end(), (size_t)0) / req.size();
    statistics_.proof_avg = std::accumulate(proof.begin(), proof.end(), (size_t)0) / proof.size();
}

bool SearchPerformanceTest::runTest() {
    std::cout << "\n================ 搜索性能测试 ================\n";
    statistics_.start_time = getCurrentTimestamp();
    auto start = std::chrono::high_resolution_clock::now();

    int total = keywords_.size();
    if (max_keywords_ > 0 && max_keywords_ < total) total = max_keywords_;

    int count = 0;
    for (const auto& kw : keywords_) {
        if (max_keywords_ > 0 && count >= max_keywords_) break;
        count++;
        std::cout << "\n[" << count << "/" << total << "] 关键词: " << kw << std::endl;
        auto r = testSingleKeyword(kw);
        results_.push_back(r);
        if (!r.success) {
            std::cerr << "⚠️  测试失败: " << r.error_msg << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    statistics_.end_time = getCurrentTimestamp();
    statistics_.total_duration_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

    calculateStatistics();

    std::cout << "\n=== 搜索测试完成 ===" << std::endl;
    std::cout << "总关键词: " << statistics_.total_keywords << " 成功: " << statistics_.success_count
              << " 失败: " << statistics_.failure_count << std::endl;
    std::cout << "客户端平均耗时: " << statistics_.t_client_avg << " ms" << std::endl;
    std::cout << "服务端平均耗时: " << statistics_.t_server_avg << " ms" << std::endl;
    std::cout << "请求大小平均: " << statistics_.request_avg << " bytes" << std::endl;
    std::cout << "证明大小平均: " << statistics_.proof_avg << " bytes" << std::endl;

    return true;
}

bool SearchPerformanceTest::saveDetailedReport(const std::string& csv_file) {
    std::ofstream ofs(csv_file);
    if (!ofs.is_open()) return false;
    ofs << "keyword,t_client_ms,t_server_ms,request_size,proof_size,result_count,timestamp,success,error_msg\n";
    for (const auto& r : results_) {
        ofs << r.keyword << "," << r.t_client_ms << "," << r.t_server_ms << "," << r.request_size
            << "," << r.proof_size << "," << r.result_count << "," << r.timestamp << ","
            << (r.success ? "true" : "false") << "," << r.error_msg << "\n";
    }
    return true;
}

bool SearchPerformanceTest::saveSummaryReport(const std::string& json_file) {
    Json::Value root;
    root["test_name"] = statistics_.test_name;
    root["start_time"] = statistics_.start_time;
    root["end_time"] = statistics_.end_time;
    root["total_duration_sec"] = statistics_.total_duration_sec;
    root["total_keywords"] = statistics_.total_keywords;
    root["success_count"] = statistics_.success_count;
    root["failure_count"] = statistics_.failure_count;
    root["t_client_avg"] = statistics_.t_client_avg;
    root["t_client_min"] = statistics_.t_client_min;
    root["t_client_max"] = statistics_.t_client_max;
    root["t_server_avg"] = statistics_.t_server_avg;
    root["t_server_min"] = statistics_.t_server_min;
    root["t_server_max"] = statistics_.t_server_max;
    root["request_avg"] = (Json::UInt64)statistics_.request_avg;
    root["proof_avg"] = (Json::UInt64)statistics_.proof_avg;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    std::ofstream ofs(json_file);
    if (!ofs.is_open()) return false;
    ofs << Json::writeString(writer, root);
    return true;
}

std::string SearchPerformanceTest::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void SearchPerformanceTest::clearPerformanceData() {
    current_times_.clear();
    current_sizes_.clear();
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
║          搜索性能测试程序                         ║
║          Search Performance Test                  ║
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
