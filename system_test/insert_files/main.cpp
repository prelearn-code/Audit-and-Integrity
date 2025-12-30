/*
 * main.cpp - 插入性能测试主程序
 *
 * 使用 InsertPerformanceTest 类进行完整的插入性能测试
 *
 * 编译:
 *   make
 *
 * 运行:
 *   ./insert_perf_test [配置文件路径]
 *   默认配置: system_test/insert_files/config/insert_test_config.json
 */

#include "insert_test.h"
#include <iostream>
#include <cstdlib>

namespace {
const char* kDefaultConfigPath = "system_test/insert_files/config/insert_test_config.json";
}

void printUsage(const char* program_name) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📊 插入性能测试工具" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    std::cout << "用法: " << program_name << " [配置文件路径]" << std::endl;
    std::cout << "\n参数:" << std::endl;
    std::cout << "  配置文件路径  - JSON格式的测试配置文件（可选）" << std::endl;
    std::cout << "                  默认: " << kDefaultConfigPath << std::endl;
    std::cout << "\n示例:" << std::endl;
    std::cout << "  " << program_name << std::endl;
    std::cout << "  " << program_name << " custom_config.json" << std::endl;
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string config_file = kDefaultConfigPath;

    if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        config_file = arg;
    } else if (argc > 2) {
        std::cerr << "❌ 错误: 参数过多" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // 打印欢迎信息
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📊 VDS 插入性能测试工具 v1.0" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    // 创建测试实例
    InsertPerformanceTest test;

    // 加载配置
    std::cout << "[阶段 1/4] 加载配置..." << std::endl;
    if (!test.loadConfig(config_file)) {
        std::cerr << "\n❌ 配置加载失败，测试中止" << std::endl;
        return 1;
    }

    // 初始化测试环境
    std::cout << "\n[阶段 2/4] 初始化测试环境..." << std::endl;
    if (!test.initialize()) {
        std::cerr << "\n❌ 初始化失败，测试中止" << std::endl;
        return 1;
    }

    // 运行测试
    std::cout << "\n[阶段 3/4] 运行插入性能测试..." << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    if (!test.runTest()) {
        std::cerr << "\n❌ 测试执行失败" << std::endl;
        return 1;
    }

    // 保存结果
    std::cout << "\n[阶段 4/4] 保存测试结果..." << std::endl;

    std::string csv_file = "system_test/insert_files/results/insert_detailed.csv";
    std::string json_file = "system_test/insert_files/results/insert_summary.json";

    if (!test.saveDetailedReport(csv_file)) {
        std::cerr << "⚠️  警告: 详细报告保存失败" << std::endl;
    } else {
        std::cout << "✅ 详细报告已保存: " << csv_file << std::endl;
    }

    if (!test.saveSummaryReport(json_file)) {
        std::cerr << "⚠️  警告: 总结报告保存失败" << std::endl;
    } else {
        std::cout << "✅ 总结报告已保存: " << json_file << std::endl;
    }

    // 打印最终总结
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "✅ 测试完成" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

    test.printSummary();

    return 0;
}
