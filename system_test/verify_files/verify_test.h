#ifndef VERIFY_PERFORMANCE_TEST_H
#define VERIFY_PERFORMANCE_TEST_H

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

class VerifyPerformanceTest {
public:
    /**
     * @brief 单个证明的验证测试结果
     */
    struct ProofVerifyResult {
        std::string keyword;
        std::string proof_file;

        // 验证性能指标
        double t_verify_ms;               // 纯证明验证时间（毫秒，不含加载）
        size_t proof_size_bytes;          // 证明文件大小（字节）
        size_t result_count;              // 证明中的文件数

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

        int total_proofs;
        int success_count;
        int failure_count;

        // 验证性能统计
        double total_verify_time_ms;      // 所有证明验证的总时间（毫秒）
        double verify_avg_ms;             // 单个证明平均验证时间
        double verify_min_ms;             // 最小验证时间
        double verify_max_ms;             // 最大验证时间
        double verify_stddev_ms;          // 验证时间标准差

        // 数据大小统计
        size_t proof_avg_bytes;
        size_t proof_total_bytes;

        // 吞吐量统计
        double verify_qps;                // 每秒验证数 (基于验证时间)
    };

    VerifyPerformanceTest();
    ~VerifyPerformanceTest();

    bool loadConfig(const std::string& config_file);
    bool initialize();
    bool runTest();
    bool saveDetailedReport(const std::string& csv_file);
    bool saveSummaryReport(const std::string& json_file);
    void printSummary();

    /**
     * @brief 清理验证测试产生的数据（如有临时文件）
     * @return 成功返回true
     */
    bool cleanupData();

private:
    // 配置
    std::string proof_dir_;               // 证明文件目录（从搜索测试生成）
    std::string public_params_file_;
    std::string private_key_file_;
    std::string client_data_dir_;
    std::string server_data_dir_;
    int server_port_;
    int max_proofs_;
    bool verbose_;
    bool save_intermediate_;

    // 组件
    StorageClient* client_;
    StorageNode* server_;

    // 数据
    std::vector<std::string> proof_files_;
    std::vector<ProofVerifyResult> results_;
    TestStatistics statistics_{};

    // 内部方法
    bool loadProofFiles();
    ProofVerifyResult testSingleProof(const std::string& proof_file);
    void calculateStatistics();
    double calculateStdDev(const std::vector<double>& values, double mean);
    std::string getCurrentTimestamp();
    bool readJson(const std::string& path, Json::Value& out);
    std::string extractKeywordFromProofFile(const std::string& proof_file);
};

#endif // VERIFY_PERFORMANCE_TEST_H
