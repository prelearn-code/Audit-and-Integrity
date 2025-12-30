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
    struct KeywordTestResult {
        std::string keyword;
        double t_client_ms;    // 令牌生成时间
        double t_server_ms;    // 服务端搜索时间
        size_t request_size;   // 请求JSON大小
        size_t proof_size;     // 证明JSON大小
        size_t result_count;   // 命中文件数
        std::string timestamp;
        bool success;
        std::string error_msg;
    };

    struct TestStatistics {
        std::string test_name;
        std::string start_time;
        std::string end_time;
        double total_duration_sec;
        int total_keywords;
        int success_count;
        int failure_count;
        double t_client_avg;
        double t_client_min;
        double t_client_max;
        double t_server_avg;
        double t_server_min;
        double t_server_max;
        size_t request_avg;
        size_t proof_avg;
    };

    SearchPerformanceTest();
    ~SearchPerformanceTest();

    bool loadConfig(const std::string& config_file);
    bool initialize();
    bool runTest();
    bool saveDetailedReport(const std::string& csv_file);
    bool saveSummaryReport(const std::string& json_file);

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

    // 组件
    StorageClient* client_;
    StorageNode* server_;
    PerformanceCallback_c callback_c_;
    PerformanceCallback_s callback_s_;

    // 数据
    std::vector<std::string> keywords_;
    std::vector<KeywordTestResult> results_;
    TestStatistics statistics_{};
    std::map<std::string, double> current_times_;
    std::map<std::string, size_t> current_sizes_;

    // 内部方法
    bool loadKeywords();
    KeywordTestResult testSingleKeyword(const std::string& keyword);
    void calculateStatistics();
    std::string getCurrentTimestamp();
    void clearPerformanceData();
    bool readJson(const std::string& path, Json::Value& out);
};

#endif // SEARCH_PERFORMANCE_TEST_H
