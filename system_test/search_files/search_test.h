#ifndef SEARCH_PERFORMANCE_TEST_H
#define SEARCH_PERFORMANCE_TEST_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <jsoncpp/json/json.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "../../vds-client/client.h"
#include "../../Storage-node/storage_node.h"

class SearchPerformanceTest {
public:
    /**
     * @brief 单个关键词的测试结果
     */
    struct KeywordTestResult {
        std::string keyword;

        // 客户端性能指标
        double t_client_token_gen_ms;     // 客户端：Token生成时间（毫秒）
        size_t token_size_bytes;           // Token大小（字节）

        // 服务端性能指标
        double t_server_proof_calc_ms;    // 服务端：纯证明计算时间（毫秒，不含加载）
        size_t proof_size_bytes;           // 证明大小（字节）
        size_t result_count;               // 命中文件数

        std::string timestamp;
        bool success;
        std::string error_msg;
    };

    /**
     * @brief 测试统计数据
     */
    struct TestStatistics {
        std::string test_name;
        std::string start_time;
        std::string end_time;
        double total_duration_sec;        // 总测试时长（秒）

        int total_keywords;
        int success_count;
        int failure_count;

        // 客户端统计
        double total_client_time_ms;      // 所有Token生成的总时间（毫秒）
        double client_token_avg_ms;       // 单个Token平均生成时间
        double client_token_min_ms;       // 最小Token生成时间
        double client_token_max_ms;       // 最大Token生成时间
        double client_token_stddev_ms;    // Token生成时间标准差

        // 服务端统计
        double total_server_time_ms;      // 所有证明计算的总时间（毫秒）
        double server_proof_avg_ms;       // 单个证明平均计算时间
        double server_proof_min_ms;       // 最小证明计算时间
        double server_proof_max_ms;       // 最大证明计算时间
        double server_proof_stddev_ms;    // 证明计算时间标准差

        // 数据大小统计
        size_t token_avg_bytes;
        size_t proof_avg_bytes;

        // 吞吐量统计
        double client_qps;                // 客户端每秒查询数 (基于token生成时间)
        double server_qps;                // 服务端每秒查询数 (基于证明计算时间)
    };

    SearchPerformanceTest();
    ~SearchPerformanceTest();

    bool loadConfig(const std::string& config_file);
    bool initialize();
    bool runTest();
    bool saveDetailedReport(const std::string& csv_file);
    bool saveSummaryReport(const std::string& json_file);
    void printSummary();

private:
    // 配置
    std::string keywords_file_;
    std::string public_params_file_;
    std::string private_key_file_;
    std::string client_data_dir_;
    std::string client_search_dir_;
    std::string keyword_states_file_;
    std::string client_insert_dir_;
    std::string client_enc_dir_;
    std::string client_meta_dir_;
    std::string client_deles_dir_;
    std::string server_data_dir_;
    std::string server_search_proof_dir_;
    int server_port_;
    int max_keywords_;
    bool verbose_;
    bool save_intermediate_;
    bool use_keyword_states_;
    bool verify_proof_;

    // 组件
    StorageClient* client_;
    StorageNode* server_;

    // 数据
    std::vector<std::string> keywords_;
    std::vector<KeywordTestResult> results_;
    TestStatistics statistics_{};

    // 内部方法
    bool loadKeywords();
    KeywordTestResult testSingleKeyword(const std::string& keyword);
    void calculateStatistics();
    double calculateStdDev(const std::vector<double>& values, double mean);
    std::string getCurrentTimestamp();
    bool readJson(const std::string& path, Json::Value& out);
};

#endif // SEARCH_PERFORMANCE_TEST_H
