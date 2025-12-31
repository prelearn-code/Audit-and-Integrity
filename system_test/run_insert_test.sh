#!/bin/bash

# ============================================================
# VDS 插入性能测试脚本（独立）
# ============================================================
#
# 功能：
# - 清理所有数据库和插入相关文件
# - 运行插入性能测试
# - 生成插入测试结果报告
#
# 用法:
#   ./run_insert_test.sh
#
# ============================================================

set -e  # 遇到错误立即退出

# ============================================================
# 配置
# ============================================================

# 脚本目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 时间戳
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# 结果目录
RESULTS_DIR="insert_results_${TIMESTAMP}"

# ============================================================
# 辅助函数
# ============================================================

print_header() {
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
}

print_step() {
    echo -e "${BLUE}▶ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# ============================================================
# 主流程
# ============================================================

main() {
    print_header "🚀 VDS 插入性能测试（独立测试）"

    echo "测试目的: 测试文件插入性能"
    echo "清理范围: 所有数据库和插入相关文件"
    echo "结果目录: $RESULTS_DIR"
    echo ""

    # 创建结果目录
    mkdir -p "$RESULTS_DIR"

    # 进入插入测试目录
    cd insert_files

    # 检查可执行文件
    if [ ! -f "insert_perf_test" ]; then
        print_step "编译插入测试程序..."
        if make clean && make; then
            print_success "编译成功"
        else
            print_error "编译失败"
            exit 1
        fi
    fi

    # 运行插入测试
    print_header "运行插入性能测试"
    print_warning "注意: 测试将自动清理所有数据库和文件"
    echo ""

    ./insert_perf_test

    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        print_success "插入性能测试完成"

        # 复制结果到结果目录
        if [ -d "results" ]; then
            cp -r results/* "../${RESULTS_DIR}/" 2>/dev/null || true
            print_success "测试结果已保存到 ${RESULTS_DIR}/"
        fi

        # 显示数据库统计
        print_header "数据库统计"

        INDEX_DB="../../Storage-node/data/index_db.json"
        SEARCH_DB="../../Storage-node/data/search_db.json"

        if [ -f "$INDEX_DB" ]; then
            SIZE=$(stat -f%z "$INDEX_DB" 2>/dev/null || stat -c%s "$INDEX_DB" 2>/dev/null || echo "unknown")
            print_success "index_db.json: $SIZE bytes"
        fi

        if [ -f "$SEARCH_DB" ]; then
            SIZE=$(stat -f%z "$SEARCH_DB" 2>/dev/null || stat -c%s "$SEARCH_DB" 2>/dev/null || echo "unknown")
            print_success "search_db.json: $SIZE bytes"
        fi

        ENC_COUNT=$(find ../../Storage-node/data/EncFiles -type f 2>/dev/null | wc -l)
        print_success "加密文件数量: $ENC_COUNT 个"

    else
        print_error "插入性能测试失败（退出码: $exit_code）"
        exit 1
    fi

    cd ..

    # 完成
    print_header "✅ 插入测试完成"
    echo "测试结果: $RESULTS_DIR"
    echo ""
    echo "下一步:"
    echo "  - 运行搜索测试: ./run_search_test.sh"
    echo "  - 运行验证测试: ./run_verify_test.sh"
    echo ""
}

# ============================================================
# 脚本入口
# ============================================================

# 显示用法
if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    echo "用法: $0"
    echo ""
    echo "功能:"
    echo "  - 清理所有数据库和插入相关文件"
    echo "  - 运行插入性能测试"
    echo "  - 生成测试结果报告"
    echo ""
    echo "注意:"
    echo "  - 此测试会删除所有现有的数据库文件"
    echo "  - 插入测试是独立的，不依赖其他测试"
    echo ""
    exit 0
fi

# 运行主流程
main

exit 0
