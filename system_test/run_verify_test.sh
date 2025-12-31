#!/bin/bash

# ============================================================
# VDS 验证性能测试脚本（独立）
# ============================================================
#
# 功能：
# - 运行证明验证性能测试（只读取，不清理）
# - 生成验证测试结果报告
#
# 前提条件：
# - 必须先运行过搜索测试（需要证明文件）
#
# 用法:
#   ./run_verify_test.sh
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
RESULTS_DIR="verify_results_${TIMESTAMP}"

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

    if [ ! -f "$INDEX_DB" ] || [ ! -f "$SEARCH_DB" ]; then
        print_error "未找到数据库文件"
        echo ""
        echo "验证测试需要数据库文件。"
        echo "请先运行插入测试："
        echo "  ./run_insert_test.sh"
        echo ""
        exit 1
    fi

    print_success "找到数据库文件"

    # 检查证明文件是否存在
    PROOF_DIR="../Storage-node/data/SearchProof"
    if [ ! -d "$PROOF_DIR" ]; then
        print_error "未找到证明文件目录: $PROOF_DIR"
        echo ""
        echo "验证测试需要搜索测试产生的证明文件。"
        echo "请先运行搜索测试："
        echo "  ./run_search_test.sh"
        echo ""
        exit 1
    fi

    PROOF_COUNT=$(find "$PROOF_DIR" -name "*.json" -type f 2>/dev/null | wc -l)
    if [ "$PROOF_COUNT" -eq 0 ]; then
        print_error "未找到证明文件"
        echo ""
        echo "验证测试需要搜索测试产生的证明文件。"
        echo "请先运行搜索测试："
        echo "  ./run_search_test.sh"
        echo ""
        exit 1
    fi

    print_success "找到 $PROOF_COUNT 个证明文件"

    print_success "依赖检查通过"
}

# ============================================================
# 主流程
# ============================================================

main() {
    print_header "✓ VDS 验证性能测试（独立测试）"

    echo "测试目的: 测试证明验证性能"
    echo "清理范围: 不需要清理（只读测试）"
    echo "依赖数据: 搜索测试产生的证明文件"
    echo "结果目录: $RESULTS_DIR"
    echo ""

    # 检查依赖
    check_dependencies

    # 创建结果目录
    mkdir -p "$RESULTS_DIR"

    # 进入验证测试目录
    cd verify_files

    # 检查可执行文件
    if [ ! -f "verify_perf_test" ]; then
        print_step "编译验证测试程序..."
        if make clean && make; then
            print_success "编译成功"
        else
            print_error "编译失败"
            exit 1
        fi
    fi

    # 运行验证测试
    print_header "运行验证性能测试"
    print_warning "注意: 验证测试只读取证明文件，不会修改任何数据"
    echo ""

    ./verify_perf_test

    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        print_success "验证性能测试完成"

        # 复制结果到结果目录
        if [ -d "results" ]; then
            cp -r results/* "../${RESULTS_DIR}/" 2>/dev/null || true
            print_success "测试结果已保存到 ${RESULTS_DIR}/"
        fi

    else
        print_error "验证性能测试失败（退出码: $exit_code）"
        exit 1
    fi

    cd ..

    # 完成
    print_header "✅ 验证测试完成"
    echo "测试结果: $RESULTS_DIR"
    echo ""
    echo "所有三个测试已完成："
    echo "  ✅ 插入测试"
    echo "  ✅ 搜索测试"
    echo "  ✅ 验证测试"
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
    echo "  - 读取搜索证明文件"
    echo "  - 运行验证性能测试"
    echo "  - 生成测试结果报告"
    echo ""
    echo "前提条件:"
    echo "  - 必须先运行过插入测试（需要数据库）"
    echo "  - 必须先运行过搜索测试（需要证明文件）"
    echo ""
    echo "注意:"
    echo "  - 此测试不会修改任何文件"
    echo "  - 此测试依赖搜索测试产生的证明文件"
    echo ""
    exit 0
fi

# 运行主流程
main

exit 0
