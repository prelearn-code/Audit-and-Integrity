#include "verify_test.h"
#include <numeric>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace fs = std::filesystem;

namespace {
const char* kDefaultConfig = "config/verify_test_config.json";
}

VerifyPerformanceTest::VerifyPerformanceTest()
    : client_(nullptr), server_(nullptr),
      server_port_(9000), max_proofs_(0),
      verbose_(true), save_intermediate_(true) {
}

VerifyPerformanceTest::~VerifyPerformanceTest() {
    delete client_;
    delete server_;
}

bool VerifyPerformanceTest::loadConfig(const std::string& config_file) {
    std::cout << "[配置] 加载配置文件: " << config_file << std::endl;

    Json::Value config;
    if (!readJson(config_file, config)) {
        std::cerr << "[错误] 无法加载配置文件: " << config_file << std::endl;
        return false;
    }

    statistics_.test_name = config.get("test_name", "verify performance test").asString();

    const Json::Value& paths = config["paths"];
    proof_dir_ = fs::path(paths.get("proof_dir", "../../Storage-node/data/SearchProof").asString()).lexically_normal().string();
    public_params_file_ = fs::path(paths.get("public_params", "../../vds-client/data/public_params.json").asString()).lexically_normal().string();
    private_key_file_ = fs::path(paths.get("private_key", "../../vds-client/data/private_key.dat").asString()).lexically_normal().string();

    const Json::Value& client_cfg = paths["client"];
    client_data_dir_ = fs::path(client_cfg.get("data_dir", "../../vds-client/data").asString()).lexically_normal().string();

    const Json::Value& server_cfg = paths["server"];
    server_data_dir_ = fs::path(server_cfg.get("data_dir", "../../Storage-node/data").asString()).lexically_normal().string();
    server_port_ = server_cfg.get("port", 9000).asInt();

    const Json::Value& options = config["options"];
    max_proofs_ = options.get("max_proofs", 0).asInt();
    verbose_ = options.get("verbose", true).asBool();
    save_intermediate_ = options.get("save_intermediate", true).asBool();

    std::cout << "[配置] 测试名称: " << statistics_.test_name << std::endl;
    std::cout << "[配置] 证明文件目录: " << proof_dir_ << std::endl;
    std::cout << "[配置] 最大证明数: " << (max_proofs_ == 0 ? "全部" : std::to_string(max_proofs_)) << std::endl;
    return true;
}

bool VerifyPerformanceTest::loadProofFiles() {
    std::cout << "[数据] 扫描证明文件: " << proof_dir_ << std::endl;

    if (!fs::exists(proof_dir_)) {
        std::cerr << "[错误] 证明文件目录不存在: " << proof_dir_ << std::endl;
        return false;
    }

    for (const auto& entry : fs::directory_iterator(proof_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            // 加载所有 .json 文件作为证明文件
            // 证明文件格式: <keyword_hash>.json 或 <keyword_hash><file_id>.json
            proof_files_.push_back(entry.path().string());
        }
    }

    if (proof_files_.empty()) {
        std::cerr << "[错误] 未找到证明文件" << std::endl;
        std::cerr << "[提示] 请先运行搜索性能测试生成证明文件" << std::endl;
        return false;
    }

    // 排序以保证顺序一致
    std::sort(proof_files_.begin(), proof_files_.end());

    // 限制数量
    if (max_proofs_ > 0 && proof_files_.size() > (size_t)max_proofs_) {
        proof_files_.resize(max_proofs_);
    }

    std::cout << "[数据] 已找到证明文件数量: " << proof_files_.size() << std::endl;
    return true;
}

bool VerifyPerformanceTest::initialize() {
    if (!loadProofFiles()) return false;

    // 初始化客户端
    std::cout << "[初始化] 初始化客户端..." << std::endl;
    client_ = new StorageClient();

    // 配置数据目录
    StorageClient::configureDataDirectories(
        client_data_dir_,
        client_data_dir_ + "/Insert",
        client_data_dir_ + "/EncFiles",
        client_data_dir_ + "/MetaFiles",
        client_data_dir_ + "/Search",
        client_data_dir_ + "/Deles",
        client_data_dir_ + "/keyword_states.json");

    if (!client_->initialize(public_params_file_)) {
        std::cerr << "[错误] 客户端初始化失败" << std::endl;
        return false;
    }

    if (!client_->initializeDataDirectories()) {
        std::cerr << "[错误] 客户端目录初始化失败" << std::endl;
        return false;
    }

    // 加载密钥
    if (!client_->loadKeys(private_key_file_)) {
        std::cout << "[初始化] 未找到密钥，生成新密钥..." << std::endl;
        if (!client_->generateKeys(private_key_file_)) {
            std::cerr << "[错误] 密钥生成失败" << std::endl;
            return false;
        }
    }

    std::cout << "[初始化] 客户端初始化完成" << std::endl;

    // 初始化服务端（提前加载数据库和索引）
    std::cout << "[初始化] 初始化服务端..." << std::endl;
    server_ = new StorageNode(server_data_dir_, server_port_);

    if (!server_->load_public_params(public_params_file_)) {
        std::cerr << "[错误] 服务端加载公共参数失败" << std::endl;
        return false;
    }

    if (!server_->initialize_directories()) {
        std::cerr << "[错误] 服务端目录初始化失败" << std::endl;
        return false;
    }

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

std::string VerifyPerformanceTest::extractKeywordFromProofFile(const std::string& proof_file) {
    fs::path p(proof_file);
    std::string filename = p.filename().string();

    // 从 "proof_keyword.json" 提取 "keyword"
    if (filename.find("proof_") == 0) {
        std::string keyword = filename.substr(6); // 移除 "proof_"
        if (keyword.size() > 5) { // 移除 ".json"
            keyword = keyword.substr(0, keyword.size() - 5);
        }
        return keyword;
    }
    return filename;
}

VerifyPerformanceTest::ProofVerifyResult VerifyPerformanceTest::testSingleProof(const std::string& proof_file) {
    ProofVerifyResult result;
    result.proof_file = proof_file;
    result.keyword = extractKeywordFromProofFile(proof_file);
    result.timestamp = getCurrentTimestamp();
    result.success = false;

    if (verbose_) {
        std::cout << "\n[测试] 证明文件: " << fs::path(proof_file).filename().string() << std::endl;
        std::cout << "  关键词: " << result.keyword << std::endl;
    }

    // 获取证明文件大小
    if (fs::exists(proof_file)) {
        result.proof_size_bytes = fs::file_size(proof_file);
        if (verbose_) {
            std::cout << "  📄 证明大小: " << result.proof_size_bytes << " bytes" << std::endl;
        }
    } else {
        result.error_msg = "证明文件不存在";
        if (verbose_) {
            std::cout << "  ❌ " << result.error_msg << std::endl;
        }
        return result;
    }

    // 读取证明文件获取文件数量（这部分不计入验证时间）
    Json::Value proof_json;
    if (readJson(proof_file, proof_json)) {
        if (proof_json.isMember("file_proofs") && proof_json["file_proofs"].isArray()) {
            result.result_count = proof_json["file_proofs"].size();
            if (verbose_) {
                std::cout << "  🔍 证明文件数: " << result.result_count << std::endl;
            }
        }
    }

    // ==================== 纯证明验证（不含文件加载） ====================
    if (verbose_) {
        std::cout << "  [验证] 开始验证证明..." << std::endl;
    }

    // 精确测量证明验证时间（数据库和参数已预加载，只测量验证过程）
    auto verify_start = std::chrono::high_resolution_clock::now();
    bool verify_success = server_->VerifySearchProof(proof_file);
    auto verify_end = std::chrono::high_resolution_clock::now();

    result.t_verify_ms = std::chrono::duration<double, std::milli>(verify_end - verify_start).count();

    if (!verify_success) {
        result.error_msg = "证明验证失败";
        if (verbose_) {
            std::cout << "  ❌ " << result.error_msg << " (" << std::fixed << std::setprecision(3)
                      << result.t_verify_ms << " ms)" << std::endl;
        }
        return result;
    }

    if (verbose_) {
        std::cout << "  ✅ 验证成功 (" << std::fixed << std::setprecision(3)
                  << result.t_verify_ms << " ms)" << std::endl;
    }

    result.success = true;
    return result;
}

bool VerifyPerformanceTest::cleanupData() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🧹 清理验证测试数据" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    // 验证测试主要是读取和验证，通常不产生额外文件
    // 但保持方法以便将来扩展或清理临时文件
    std::cout << "[清理] 验证测试不产生需要清理的数据" << std::endl;
    std::cout << "[清理] 验证测试只读取证明文件进行验证\n" << std::endl;

    std::cout << "✅ 清理完成\n" << std::endl;
    return true;
}

bool VerifyPerformanceTest::runTest() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "开始证明验证性能测试" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    // 清理验证测试数据（如有）
    if (!cleanupData()) {
        std::cerr << "❌ 验证数据清理失败" << std::endl;
        return false;
    }

    statistics_.start_time = getCurrentTimestamp();
    auto test_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < proof_files_.size(); i++) {
        std::cout << "\n进度: [" << (i + 1) << "/" << proof_files_.size() << "]" << std::endl;

        ProofVerifyResult result = testSingleProof(proof_files_[i]);
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
    statistics_.total_proofs = proof_files_.size();

    calculateStatistics();

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "测试完成" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    printSummary();

    return true;
}

void VerifyPerformanceTest::calculateStatistics() {
    if (results_.empty()) return;

    std::vector<double> verify_times;
    std::vector<size_t> proof_sizes;

    for (const auto& r : results_) {
        if (r.success) {
            verify_times.push_back(r.t_verify_ms);
            proof_sizes.push_back(r.proof_size_bytes);
        }
    }

    if (!verify_times.empty()) {
        // 验证性能统计
        statistics_.total_verify_time_ms = std::accumulate(verify_times.begin(), verify_times.end(), 0.0);
        statistics_.verify_avg_ms = statistics_.total_verify_time_ms / verify_times.size();
        statistics_.verify_min_ms = *std::min_element(verify_times.begin(), verify_times.end());
        statistics_.verify_max_ms = *std::max_element(verify_times.begin(), verify_times.end());
        statistics_.verify_stddev_ms = calculateStdDev(verify_times, statistics_.verify_avg_ms);
        statistics_.verify_qps = (statistics_.total_verify_time_ms > 0) ?
            (verify_times.size() * 1000.0 / statistics_.total_verify_time_ms) : 0.0;

        // 数据大小统计
        statistics_.proof_avg_bytes = std::accumulate(proof_sizes.begin(), proof_sizes.end(), 0UL) / proof_sizes.size();
        statistics_.proof_total_bytes = std::accumulate(proof_sizes.begin(), proof_sizes.end(), 0UL);
    }
}

double VerifyPerformanceTest::calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) return 0.0;

    double sum_sq_diff = 0.0;
    for (double v : values) {
        double diff = v - mean;
        sum_sq_diff += diff * diff;
    }
    return std::sqrt(sum_sq_diff / (values.size() - 1));
}

void VerifyPerformanceTest::printSummary() {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📊 验证性能测试总结" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    std::cout << "测试名称: " << statistics_.test_name << std::endl;
    std::cout << "开始时间: " << statistics_.start_time << std::endl;
    std::cout << "结束时间: " << statistics_.end_time << std::endl;
    std::cout << "总耗时: " << std::fixed << std::setprecision(2)
              << statistics_.total_duration_sec << " 秒" << std::endl;
    std::cout << "总证明数: " << statistics_.total_proofs << std::endl;
    std::cout << "成功: " << statistics_.success_count << " | 失败: " << statistics_.failure_count << std::endl;

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "✅ 验证性能（纯验证时间，不含加载）" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "总验证时间: " << std::fixed << std::setprecision(2)
              << statistics_.total_verify_time_ms << " ms" << std::endl;
    std::cout << "平均验证时间: " << std::fixed << std::setprecision(3)
              << statistics_.verify_avg_ms << " ms" << std::endl;
    std::cout << "最小验证时间: " << std::fixed << std::setprecision(3)
              << statistics_.verify_min_ms << " ms" << std::endl;
    std::cout << "最大验证时间: " << std::fixed << std::setprecision(3)
              << statistics_.verify_max_ms << " ms" << std::endl;
    std::cout << "标准差: " << std::fixed << std::setprecision(3)
              << statistics_.verify_stddev_ms << " ms" << std::endl;
    std::cout << "验证吞吐量: " << std::fixed << std::setprecision(2)
              << statistics_.verify_qps << " 验证/秒" << std::endl;

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📦 数据大小统计" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "平均证明大小: " << statistics_.proof_avg_bytes << " bytes" << std::endl;
    std::cout << "总证明大小: " << statistics_.proof_total_bytes << " bytes" << std::endl;
    std::cout << std::endl;
}

bool VerifyPerformanceTest::saveDetailedReport(const std::string& csv_file) {
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
    ofs << "keyword,proof_file,"
        << "verify_time_ms,proof_size_bytes,result_count,"
        << "timestamp,success,error_msg\n";

    // 数据行
    for (const auto& r : results_) {
        ofs << r.keyword << ","
            << fs::path(r.proof_file).filename().string() << ","
            << std::fixed << std::setprecision(6) << r.t_verify_ms << ","
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

bool VerifyPerformanceTest::saveSummaryReport(const std::string& json_file) {
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
    root["test_info"]["total_proofs"] = statistics_.total_proofs;
    root["test_info"]["success_count"] = statistics_.success_count;
    root["test_info"]["failure_count"] = statistics_.failure_count;

    // 验证性能统计
    root["verify_performance"]["total_time_ms"] = statistics_.total_verify_time_ms;
    root["verify_performance"]["verify_avg_ms"] = statistics_.verify_avg_ms;
    root["verify_performance"]["verify_min_ms"] = statistics_.verify_min_ms;
    root["verify_performance"]["verify_max_ms"] = statistics_.verify_max_ms;
    root["verify_performance"]["verify_stddev_ms"] = statistics_.verify_stddev_ms;
    root["verify_performance"]["qps"] = statistics_.verify_qps;
    root["verify_performance"]["note"] = "Pure verification time, excluding database and file loading";

    // 数据大小
    root["data_size"]["proof_avg_bytes"] = (Json::Value::UInt64)statistics_.proof_avg_bytes;
    root["data_size"]["proof_total_bytes"] = (Json::Value::UInt64)statistics_.proof_total_bytes;

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

std::string VerifyPerformanceTest::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool VerifyPerformanceTest::readJson(const std::string& path, Json::Value& out) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }
    Json::CharReaderBuilder builder;
    std::string errs;
    return Json::parseFromStream(builder, ifs, &out, &errs);
}

// ============================================================
// Main 函数
// ============================================================

int main(int argc, char* argv[]) {
    std::cout << R"(
╔══════════════════════════════════════════════════╗
║   VDS 搜索证明验证性能测试                        ║
║   Verify Performance Test                        ║
╚══════════════════════════════════════════════════╝
)" << std::endl;

    std::string config_file = kDefaultConfig;
    if (argc > 1) {
        config_file = argv[1];
    }

    VerifyPerformanceTest test;

    if (!test.loadConfig(config_file)) {
        std::cerr << "\n❌ 配置加载失败" << std::endl;
        return 1;
    }

    if (!test.initialize()) {
        std::cerr << "\n❌ 初始化失败" << std::endl;
        return 1;
    }

    if (!test.runTest()) {
        std::cerr << "\n❌ 测试执行失败" << std::endl;
        return 1;
    }

    // 保存结果
    std::string csv_file = "results/verify_detailed.csv";
    std::string json_file = "results/verify_summary.json";

    if (!test.saveDetailedReport(csv_file)) {
        std::cerr << "\n⚠️  保存详细报告失败" << std::endl;
    }

    if (!test.saveSummaryReport(json_file)) {
        std::cerr << "\n⚠️  保存总结报告失败" << std::endl;
    }

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "✅ 所有测试完成！" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "\n📊 结果文件:" << std::endl;
    std::cout << "  - 详细报告 (CSV): " << csv_file << std::endl;
    std::cout << "  - 总结报告 (JSON): " << json_file << std::endl;
    std::cout << std::endl;

    return 0;
}
