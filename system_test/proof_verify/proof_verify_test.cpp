#include "./proof_verify_test.h"
#include <numeric>
#include <sstream>

namespace fs = std::filesystem;

ProofVerifyPerformanceTest::ProofVerifyPerformanceTest()
    : max_proofs_(0), verbose_(true), server_(nullptr) {
    callback_s_.on_phase_complete = [this](const std::string& name, double time_ms) {
        if (verbose_) {
            std::cout << "  [TIME] " << name << ": " << time_ms << " ms" << std::endl;
        }
        if (name == "server_search_verify_total") {
            last_server_verify_ms_ = time_ms;
        }
    };
}

ProofVerifyPerformanceTest::~ProofVerifyPerformanceTest() {
    delete server_;
}

bool ProofVerifyPerformanceTest::loadConfig(const std::string& config_file) {
    std::cout << "\n[配置] 加载验证测试配置: " << config_file << std::endl;
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
    public_params_file_ = fs::path(paths.get("public_params", "").asString()).lexically_normal().string();
    server_data_dir_ = fs::path(paths.get("server_data_dir", "Storage-node/data").asString()).lexically_normal().string();
    proof_dir_ = fs::path(paths.get("proof_dir", server_data_dir_ + "/SearchProof").asString()).lexically_normal().string();

    const Json::Value& options = config["options"];
    max_proofs_ = options.get("max_proofs", 0).asInt();
    verbose_ = options.get("verbose", true).asBool();

    stats_.test_name = config.get("test_name", "proof_verify_performance").asString();

    if (!fs::exists(proof_dir_)) {
        std::cerr << "[错误] 证明目录不存在: " << proof_dir_ << std::endl;
        return false;
    }
    return true;
}

bool ProofVerifyPerformanceTest::loadProofFiles() {
    proof_files_.clear();
    for (const auto& entry : fs::directory_iterator(proof_dir_)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".json") {
            proof_files_.push_back(entry.path().lexically_normal().string());
        }
    }
    if (proof_files_.empty()) {
        std::cerr << "[警告] 未找到任何证明文件" << std::endl;
        return false;
    }
    std::sort(proof_files_.begin(), proof_files_.end());
    std::cout << "[数据] 发现证明文件: " << proof_files_.size() << std::endl;
    return true;
}

bool ProofVerifyPerformanceTest::initialize() {
    server_ = new StorageNode(server_data_dir_, 0);
    if (!server_->load_public_params(public_params_file_)) {
        std::cerr << "[错误] 服务端加载公共参数失败" << std::endl;
        return false;
    }
    if (!server_->initialize_directories()) {
        std::cerr << "[错误] 服务端目录初始化失败" << std::endl;
        return false;
    }
    server_->setPerformanceCallback_s(&callback_s_);
    return loadProofFiles();
}

ProofVerifyPerformanceTest::VerifyResult ProofVerifyPerformanceTest::verifySingle(const std::string& proof_path) {
    VerifyResult r{};
    r.proof_file = proof_path;
    r.timestamp = getCurrentTimestamp();

    last_server_verify_ms_ = 0.0;
    if (!server_->VerifySearchProof(proof_path)) {
        r.error_msg = "验证失败";
        r.success = false;
        return r;
    }
    r.t_server_ms = last_server_verify_ms_;
    r.success = true;
    return r;
}

bool ProofVerifyPerformanceTest::runTest() {
    std::cout << "\n================ 证明验证性能测试 ================\n";
    stats_.start_time = getCurrentTimestamp();
    auto start = std::chrono::high_resolution_clock::now();

    int total = proof_files_.size();
    if (max_proofs_ > 0 && max_proofs_ < total) total = max_proofs_;

    int count = 0;
    for (const auto& proof : proof_files_) {
        if (max_proofs_ > 0 && count >= max_proofs_) break;
        count++;
        std::cout << "\n[" << count << "/" << total << "] 证明: " << proof << std::endl;
        auto res = verifySingle(proof);
        results_.push_back(res);
        if (!res.success) {
            std::cerr << "⚠️  验证失败: " << res.error_msg << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats_.end_time = getCurrentTimestamp();
    stats_.total_duration_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

    // 统计
    std::vector<double> tvals;
    for (const auto& r : results_) {
        if (r.success) tvals.push_back(r.t_server_ms);
    }
    stats_.total_proofs = results_.size();
    stats_.success_count = tvals.size();
    stats_.failure_count = results_.size() - tvals.size();
    if (!tvals.empty()) {
        stats_.t_server_avg = std::accumulate(tvals.begin(), tvals.end(), 0.0) / tvals.size();
        stats_.t_server_min = *std::min_element(tvals.begin(), tvals.end());
        stats_.t_server_max = *std::max_element(tvals.begin(), tvals.end());
    }

    std::cout << "\n=== 验证测试完成 ===" << std::endl;
    std::cout << "总证明: " << stats_.total_proofs << " 成功: " << stats_.success_count
              << " 失败: " << stats_.failure_count << std::endl;
    std::cout << "服务端平均耗时: " << stats_.t_server_avg << " ms" << std::endl;

    return true;
}

bool ProofVerifyPerformanceTest::saveDetailedReport(const std::string& csv_file) {
    std::ofstream ofs(csv_file);
    if (!ofs.is_open()) return false;
    ofs << "proof_file,t_server_ms,timestamp,success,error_msg\n";
    for (const auto& r : results_) {
        ofs << r.proof_file << "," << r.t_server_ms << "," << r.timestamp << ","
            << (r.success ? "true" : "false") << "," << r.error_msg << "\n";
    }
    return true;
}

bool ProofVerifyPerformanceTest::saveSummaryReport(const std::string& json_file) {
    Json::Value root;
    root["test_name"] = stats_.test_name;
    root["start_time"] = stats_.start_time;
    root["end_time"] = stats_.end_time;
    root["total_duration_sec"] = stats_.total_duration_sec;
    root["total_proofs"] = stats_.total_proofs;
    root["success_count"] = stats_.success_count;
    root["failure_count"] = stats_.failure_count;
    root["t_server_avg"] = stats_.t_server_avg;
    root["t_server_min"] = stats_.t_server_min;
    root["t_server_max"] = stats_.t_server_max;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    std::ofstream ofs(json_file);
    if (!ofs.is_open()) return false;
    ofs << Json::writeString(writer, root);
    return true;
}

std::string ProofVerifyPerformanceTest::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════╗
║      搜索证明验证性能测试程序                     ║
║      Search Proof Verify Performance Test        ║
╚══════════════════════════════════════════════════╝
)" << std::endl;

    ProofVerifyPerformanceTest test;
    if (!test.loadConfig("system_test/proof_verify/config/verify_test_config.json")) {
        return 1;
    }
    if (!test.initialize()) {
        return 1;
    }
    if (!test.runTest()) {
        return 1;
    }
    test.saveDetailedReport("proof_verify_report.csv");
    test.saveSummaryReport("proof_verify_summary.json");
    return 0;
}
