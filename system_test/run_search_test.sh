#!/bin/bash

# ============================================================
# VDS 搜索性能测试脚本（独立）
# ============================================================
#
# 功能：
# - 清理搜索token和proof文件（不清理数据库）
# - 运行搜索性能测试
# - 生成搜索测试结果报告
#
# 前提条件：
# - 必须先运行过插入测试（需要数据库）
#
# 用法:
#   ./run_search_test.sh
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
RESULTS_DIR="search_results_${TIMESTAMP}"

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
# 依赖检查
# ============================================================

check_dependencies() {
    print_header "检查依赖"

    # 检查数据库文件是否存在
    INDEX_DB="../Storage-node/data/index_db.json"
    SEARCH_DB="../Storage-node/data/search_db.json"

    if [ ! -f "$INDEX_DB" ]; then
        print_error "未找到索引数据库: $INDEX_DB"
        echo ""
        echo "搜索测试需要插入测试产生的数据库文件。"
        echo "请先运行插入测试："
        echo "  ./run_insert_test.sh"
        echo ""
        exit 1
    fi

    if [ ! -f "$SEARCH_DB" ]; then
        print_error "未找到搜索数据库: $SEARCH_DB"
        echo ""
        echo "搜索测试需要插入测试产生的数据库文件。"
        echo "请先运行插入测试："
        echo "  ./run_insert_test.sh"
        echo ""
        exit 1
    fi

    print_success "找到索引数据库"
    print_success "找到搜索数据库"

    # 显示数据库大小
    INDEX_SIZE=$(stat -f%z "$INDEX_DB" 2>/dev/null || stat -c%s "$INDEX_DB" 2>/dev/null || echo "unknown")
    SEARCH_SIZE=$(stat -f%z "$SEARCH_DB" 2>/dev/null || stat -c%s "$SEARCH_DB" 2>/dev/null || echo "unknown")

    echo "  索引数据库大小: $INDEX_SIZE bytes"
    echo "  搜索数据库大小: $SEARCH_SIZE bytes"

    print_success "依赖检查通过"
}

# ============================================================
# 主流程
# ============================================================

main() {
    print_header "🔍 VDS 搜索性能测试（独立测试）"

    echo "测试目的: 测试关键词搜索性能"
    echo "清理范围: 只清理搜索token和proof文件"
    echo "依赖数据: 插入测试产生的数据库"
    echo "结果目录: $RESULTS_DIR"
    echo ""

    # 检查依赖
    check_dependencies

    # 创建结果目录
    mkdir -p "$RESULTS_DIR"

    # 进入搜索测试目录
    cd search_files

    # 检查可执行文件
    if [ ! -f "search_perf_test" ]; then
        print_step "编译搜索测试程序..."
        if make clean && make; then
            print_success "编译成功"
        else
            print_error "编译失败"
            exit 1
        fi
    fi

    # 运行搜索测试
    print_header "运行搜索性能测试"
    print_warning "注意: 测试将清理之前的搜索token和proof文件"
    print_warning "注意: 数据库文件不会被清理"
    echo ""

    ./search_perf_test

    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        print_success "搜索性能测试完成"

        # 复制结果到结果目录
        if [ -d "results" ]; then
            cp -r results/* "../${RESULTS_DIR}/" 2>/dev/null || true
            print_success "测试结果已保存到 ${RESULTS_DIR}/"
        fi

        # 显示证明文件统计
        print_header "证明文件统计"

        PROOF_DIR="../../Storage-node/data/SearchProof"
        if [ -d "$PROOF_DIR" ]; then
            PROOF_COUNT=$(find "$PROOF_DIR" -name "*.json" -type f 2>/dev/null | wc -l)
            print_success "生成证明文件: $PROOF_COUNT 个"
        fi

    else
        print_error "搜索性能测试失败（退出码: $exit_code）"
        exit 1
    fi

    cd ..

    # 完成
    print_header "✅ 搜索测试完成"
    echo "测试结果: $RESULTS_DIR"
    echo ""
    echo "下一步:"
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
    echo "  - 清理搜索token和proof文件（保留数据库）"
    echo "  - 运行搜索性能测试"
    echo "  - 生成测试结果报告"
    echo ""
    echo "前提条件:"
    echo "  - 必须先运行过插入测试"
    echo "  - 需要存在 index_db.json 和 search_db.json"
    echo ""
    echo "注意:"
    echo "  - 此测试不会删除数据库文件"
    echo "  - 此测试依赖插入测试产生的数据"
    echo ""
    exit 0
fi

# 运行主流程
main

exit 0
