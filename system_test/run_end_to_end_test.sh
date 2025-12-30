#!/bin/bash

# ============================================================
# VDS 端到端性能测试脚本
# ============================================================
#
# 功能：
# 1. 运行插入性能测试
# 2. 运行搜索性能测试（使用插入测试生成的数据）
# 3. 生成综合报告
#
# 用法:
#   ./run_end_to_end_test.sh [mode]
#
# 模式:
#   quick    - 快速测试（使用少量数据）
#   standard - 标准测试（默认）
#   full     - 完整测试（使用所有数据）
#
# ============================================================

set -e  # 遇到错误立即退出

# ============================================================
# 配置
# ============================================================

# 脚本目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# 测试模式
MODE="${1:-standard}"

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
RESULTS_DIR="end_to_end_results_${TIMESTAMP}"

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

    local missing_deps=0

    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        print_error "未找到 g++ 编译器"
        missing_deps=$((missing_deps + 1))
    else
        print_success "g++ 编译器: $(g++ --version | head -1)"
    fi

    # 检查 make
    if ! command -v make &> /dev/null; then
        print_error "未找到 make 工具"
        missing_deps=$((missing_deps + 1))
    else
        print_success "make: $(make --version | head -1)"
    fi

    # 检查 jq (可选，用于解析JSON)
    if ! command -v jq &> /dev/null; then
        print_warning "未找到 jq 工具（结果展示可能不完整）"
    else
        print_success "jq: $(jq --version)"
    fi

    if [ $missing_deps -gt 0 ]; then
        print_error "缺少必要依赖，测试中止"
        exit 1
    fi

    print_success "依赖检查通过"
}

# ============================================================
# 编译测试程序
# ============================================================

build_tests() {
    print_header "编译测试程序"

    # 编译插入测试
    print_step "编译插入性能测试..."
    cd insert_files
    if make clean && make; then
        print_success "插入测试编译成功"
    else
        print_error "插入测试编译失败"
        exit 1
    fi
    cd ..

    # 编译搜索测试
    print_step "编译搜索性能测试..."
    cd search_files
    if make clean && make; then
        print_success "搜索测试编译成功"
    else
        print_error "搜索测试编译失败"
        exit 1
    fi
    cd ..

    print_success "所有测试程序编译完成"
}

# ============================================================
# 运行插入性能测试
# ============================================================

run_insert_test() {
    print_header "运行插入性能测试"

    cd insert_files

    # 根据模式选择配置
    case "$MODE" in
        quick)
            print_step "使用快速测试配置..."
            # 可以创建一个临时配置文件，限制文件数量
            ./insert_perf_test
            ;;
        full)
            print_step "使用完整测试配置..."
            ./insert_perf_test
            ;;
        *)
            print_step "使用标准测试配置..."
            ./insert_perf_test
            ;;
    esac

    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        print_success "插入性能测试完成"

        # 复制结果到综合结果目录
        if [ -d "results" ]; then
            mkdir -p "../${RESULTS_DIR}/insert"
            cp results/* "../${RESULTS_DIR}/insert/" 2>/dev/null || true
            print_success "插入测试结果已保存到 ${RESULTS_DIR}/insert/"
        fi
    else
        print_error "插入性能测试失败（退出码: $exit_code）"
        cd ..
        return 1
    fi

    cd ..
    return 0
}

# ============================================================
# 运行搜索性能测试
# ============================================================

run_search_test() {
    print_header "运行搜索性能测试"

    cd search_files

    # 检查是否存在插入测试生成的 keyword_states.json
    local keyword_states="../../vds-client/data/keyword_states.json"
    if [ -f "$keyword_states" ]; then
        print_step "检测到插入测试生成的 keyword_states.json"
        print_step "搜索测试将使用这些关键词..."
    else
        print_warning "未找到 keyword_states.json，使用配置文件中的关键词列表"
    fi

    # 运行搜索测试
    ./search_perf_test

    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        print_success "搜索性能测试完成"

        # 复制结果到综合结果目录
        if [ -d "results" ]; then
            mkdir -p "../${RESULTS_DIR}/search"
            cp results/* "../${RESULTS_DIR}/search/" 2>/dev/null || true
            print_success "搜索测试结果已保存到 ${RESULTS_DIR}/search/"
        fi
    else
        print_error "搜索性能测试失败（退出码: $exit_code）"
        cd ..
        return 1
    fi

    cd ..
    return 0
}

# ============================================================
# 生成综合报告
# ============================================================

generate_report() {
    print_header "生成综合报告"

    local report_file="${RESULTS_DIR}/summary_report.md"

    cat > "$report_file" << EOF
# VDS 端到端性能测试报告

**测试时间**: $(date +"%Y-%m-%d %H:%M:%S")
**测试模式**: $MODE

---

## 1. 测试概览

本次测试包含以下阶段:
1. **插入性能测试**: 批量插入文件并收集性能指标
2. **搜索性能测试**: 搜索已插入的文件并测试性能

---

## 2. 插入性能测试结果

EOF

    # 插入测试结果
    if [ -f "${RESULTS_DIR}/insert/insert_summary.json" ]; then
        if command -v jq &> /dev/null; then
            echo "### 统计摘要" >> "$report_file"
            echo '```json' >> "$report_file"
            jq '.' "${RESULTS_DIR}/insert/insert_summary.json" >> "$report_file"
            echo '```' >> "$report_file"
        else
            echo "详细结果请查看: \`${RESULTS_DIR}/insert/insert_summary.json\`" >> "$report_file"
        fi
    else
        echo "❌ 未找到插入测试结果" >> "$report_file"
    fi

    cat >> "$report_file" << EOF

---

## 3. 搜索性能测试结果

EOF

    # 搜索测试结果
    if [ -f "${RESULTS_DIR}/search/search_summary.json" ]; then
        if command -v jq &> /dev/null; then
            echo "### 统计摘要" >> "$report_file"
            echo '```json' >> "$report_file"
            jq '.' "${RESULTS_DIR}/search/search_summary.json" >> "$report_file"
            echo '```' >> "$report_file"
        else
            echo "详细结果请查看: \`${RESULTS_DIR}/search/search_summary.json\`" >> "$report_file"
        fi
    else
        echo "❌ 未找到搜索测试结果" >> "$report_file"
    fi

    cat >> "$report_file" << EOF

---

## 4. 文件列表

- 插入测试详细数据: \`${RESULTS_DIR}/insert/insert_detailed.csv\`
- 插入测试摘要: \`${RESULTS_DIR}/insert/insert_summary.json\`
- 搜索测试详细数据: \`${RESULTS_DIR}/search/search_detailed.csv\`
- 搜索测试摘要: \`${RESULTS_DIR}/search/search_summary.json\`

---

**测试完成时间**: $(date +"%Y-%m-%d %H:%M:%S")
EOF

    print_success "综合报告已生成: $report_file"

    # 打印报告内容
    if command -v cat &> /dev/null; then
        echo ""
        cat "$report_file"
    fi
}

# ============================================================
# 主流程
# ============================================================

main() {
    print_header "🚀 VDS 端到端性能测试"

    echo "测试模式: $MODE"
    echo "结果目录: $RESULTS_DIR"
    echo ""

    # 创建结果目录
    mkdir -p "$RESULTS_DIR"

    # 1. 检查依赖
    check_dependencies

    # 2. 编译测试程序
    build_tests

    # 3. 运行插入测试
    if ! run_insert_test; then
        print_error "插入测试失败，中止后续测试"
        exit 1
    fi

    # 4. 运行搜索测试
    if ! run_search_test; then
        print_error "搜索测试失败"
        # 继续生成报告，即使搜索测试失败
    fi

    # 5. 生成综合报告
    generate_report

    # 完成
    print_header "✅ 端到端测试完成"
    echo "所有结果已保存到: $RESULTS_DIR"
    echo ""
}

# ============================================================
# 脚本入口
# ============================================================

# 显示用法
if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    echo "用法: $0 [mode]"
    echo ""
    echo "模式:"
    echo "  quick    - 快速测试"
    echo "  standard - 标准测试（默认）"
    echo "  full     - 完整测试"
    echo ""
    exit 0
fi

# 运行主流程
main

exit 0
