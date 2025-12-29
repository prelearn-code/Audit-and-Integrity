#ifndef INSERT_PERFORMANCE_TEST_H
#define INSERT_PERFORMANCE_TEST_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <jsoncpp/json/json.h>
#include "../../vds-client/client.h"
#include "../../Storage-node/storage_node.h"

/**
 * @brief 插入操作性能测试类
 * 
 * 功能：
 * 1. 加载文件-关键词映射JSON
 * 2. 批量测试文件插入操作
 * 3. 监控客户端和服务端性能
 * 4. 生成详细统计报告
 */
class InsertPerformanceTest {
public:
    /**
     * @brief 单个文件的测试结果
     */
    struct FileTestResult {
        std::string file_id;           // 文件ID
        std::string file_path;         // 文件路径
        size_t file_size;              // 原始文件大小（字节）
        int keyword_count;             // 关键词数量
        
        // 时间指标（毫秒）
        double t1_ms;                  // 客户端加密总时间
        double t3_ms;                  // 服务端插入总时间
        
        // 数据大小指标（字节）
        size_t s1_bytes;               // 明文大小
        size_t s2_bytes;               // 密文大小
        size_t s3_bytes;               // insert.json大小
        
        // 衍生指标
        double encrypt_ratio;          // 加密膨胀率
        double metadata_ratio;         // 元数据占比
        double total_overhead;         // 总开销
        double client_throughput_mbps; // 客户端吞吐量(MB/s)
        double server_throughput_mbps; // 服务端吞吐量(MB/s)
        
        std::string timestamp;         // 测试时间戳
        bool success;                  // 是否成功
        std::string error_msg;         // 错误信息
    };
    
    /**
     * @brief 测试统计数据
     */
    struct TestStatistics {
        // 测试信息
        std::string test_name;
        std::string start_time;
        std::string end_time;
        double total_duration_sec;
        int total_files;
        int success_count;
        int failure_count;
        
        // 时间统计（毫秒）
        double t1_avg, t1_min, t1_max, t1_stddev;
        double t3_avg, t3_min, t3_max, t3_stddev;
        
        // 数据大小统计（字节）
        size_t s1_avg, s1_total;
        size_t s2_avg, s2_total;
        size_t s3_avg, s3_total;
        
        // 比率统计
        double encrypt_ratio_avg;
        double metadata_ratio_avg;
        double total_overhead_avg;
        
        // 吞吐量统计（MB/s）
        double client_throughput_avg;
        double server_throughput_avg;
        
        // 按大小分组统计
        std::map<std::string, std::map<std::string, double>> size_groups;
    };
    
    /**
     * @brief 构造函数
     */
    InsertPerformanceTest();
    
    /**
     * @brief 析构函数
     */
    ~InsertPerformanceTest();
    
    /**
     * @brief 加载测试配置文件
     * @param config_file 配置文件路径
     * @return 成功返回true
     */
    bool loadConfig(const std::string& config_file);
    
    /**
     * @brief 初始化测试环境
     * @return 成功返回true
     */
    bool initialize();
    
    /**
     * @brief 运行完整测试
     * @return 成功返回true
     */
    bool runTest();
    
    /**
     * @brief 保存详细报告（CSV格式）
     * @param csv_file 输出CSV文件路径
     * @return 成功返回true
     */
    bool saveDetailedReport(const std::string& csv_file);
    
    /**
     * @brief 保存总结报告（JSON格式）
     * @param json_file 输出JSON文件路径
     * @return 成功返回true
     */
    bool saveSummaryReport(const std::string& json_file);
    
    /**
     * @brief 打印测试进度
     */
    void printProgress(int current, int total);
    
    /**
     * @brief 打印测试总结
     */
    void printSummary();

private:
    // ==================== 配置参数 ====================
    std::string keywords_file_;        // 文件-关键词映射JSON路径
    std::string base_dir_;             // 数据基础目录
    std::string public_params_file_;   // 公共参数文件
    std::string client_data_dir_;      // 客户端数据目录
    std::string server_data_dir_;      // 服务端数据目录
    int server_port_;                  // 服务端端口
    int max_files_;                    // 最大测试文件数（0=全部）
    bool verbose_;                     // 是否显示详细日志
    bool save_intermediate_;           // 是否保存中间文件
    
    // ==================== 核心组件 ====================
    StorageClient* client_;            // 客户端实例
    StorageNode* server_;              // 服务端实例
    PerformanceCallback_c callback_c;
    PerformanceCallback_s callback_s;
    
    // ==================== 数据存储 ====================
    // 文件路径 -> 关键词列表
    std::map<std::string, std::vector<std::string>> file_keywords_map_;
    
    // 测试结果
    std::vector<FileTestResult> results_;
    
    // 统计数据
    TestStatistics statistics_;
    
    // 当前性能数据（临时存储）
    std::map<std::string, double> current_times_;
    std::map<std::string, size_t> current_sizes_;
    
    // ==================== 私有方法 ====================
    
    /**
     * @brief 加载文件-关键词映射
     * @return 成功返回true
     */
    bool loadKeywordsMapping();
    
    /**
     * @brief 测试单个文件插入
     * @param file_path 文件路径
     * @param keywords 关键词列表
     * @return 测试结果
     */
    FileTestResult testSingleFile(const std::string& file_path, 
                                  const std::vector<std::string>& keywords);
    
    /**
     * @brief 计算统计数据
     */
    void calculateStatistics();
    
    /**
     * @brief 计算标准差
     * @param values 数据列表
     * @param mean 平均值
     * @return 标准差
     */
    double calculateStdDev(const std::vector<double>& values, double mean);
    
    /**
     * @brief 获取文件大小分组名称
     * @param size 文件大小（字节）
     * @return 分组名称
     */
    std::string getSizeGroup(size_t size);
    
    /**
     * @brief 获取当前时间戳字符串
     * @return 时间戳字符串
     */
    std::string getCurrentTimestamp();
    
    /**
     * @brief 清理性能监控数据
     */
    void clearPerformanceData();
};

#endif // INSERT_PERFORMANCE_TEST_H