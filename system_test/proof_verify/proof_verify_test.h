#ifndef SEARCH_PROOF_VERIFY_TEST_H
#define SEARCH_PROOF_VERIFY_TEST_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <jsoncpp/json/json.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "../../Storage-node/storage_node.h"

class ProofVerifyPerformanceTest {
public:
    struct VerifyResult {
        std::string proof_file;
        double t_server_ms;
        bool success;
        std::string error_msg;
        std::string timestamp;
    };

    struct Statistics {
        std::string test_name;
        std::string start_time;
        std::string end_time;
        double total_duration_sec{0};
        int total_proofs{0};
        int success_count{0};
        int failure_count{0};
        double t_server_avg{0};
        double t_server_min{0};
        double t_server_max{0};
    };

    ProofVerifyPerformanceTest();
    ~ProofVerifyPerformanceTest();

    bool loadConfig(const std::string& config_file);
    bool initialize();
    bool runTest();
    bool saveDetailedReport(const std::string& csv_file);
    bool saveSummaryReport(const std::string& json_file);

private:
    std::string public_params_file_;
    std::string server_data_dir_;
    std::string proof_dir_;
    int max_proofs_;
    bool verbose_;
    double last_server_verify_ms_{0.0};

    StorageNode* server_;
    PerformanceCallback_s callback_s_;

    std::vector<std::string> proof_files_;
    std::vector<VerifyResult> results_;
    Statistics stats_{};

    bool loadProofFiles();
    VerifyResult verifySingle(const std::string& proof_path);
    std::string getCurrentTimestamp();
};

#endif // SEARCH_PROOF_VERIFY_TEST_H
