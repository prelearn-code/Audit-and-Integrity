#!/bin/bash

# ============================================================
# 配置文件路径验证脚本
# ============================================================

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔍 验证测试配置文件路径"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

check_file() {
    local desc=$1
    local file=$2

    if [ -f "$file" ]; then
        echo -e "${GREEN}✅ $desc: $file${NC}"
        return 0
    else
        echo -e "${RED}❌ $desc: $file (不存在)${NC}"
        return 1
    fi
}

check_dir() {
    local desc=$1
    local dir=$2

    if [ -d "$dir" ]; then
        echo -e "${GREEN}✅ $desc: $dir${NC}"
        return 0
    else
        echo -e "${RED}❌ $desc: $dir (不存在)${NC}"
        return 1
    fi
}

failed=0

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📝 插入测试配置验证 (从 insert_files/ 运行)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

cd system_test/insert_files

check_file "配置文件" "config/insert_test_config.json" || failed=1
check_file "关键词文件" "data/database1_keywords.json" || failed=1
check_file "公共参数" "../../vds-client/data/public_params.json" || failed=1
check_file "私钥文件" "../../vds-client/data/private_key.dat" || failed=1
check_dir "数据集根目录" "../../make_data/database1" || failed=1
check_dir "客户端数据目录" "../../vds-client/data" || failed=1
check_dir "客户端Insert目录" "../../vds-client/data/Insert" || failed=1
check_dir "客户端EncFiles目录" "../../vds-client/data/EncFiles" || failed=1
check_dir "服务端数据目录" "../../Storage-node/data" || failed=1

cd ../..

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔍 搜索测试配置验证 (从 search_files/ 运行)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

cd system_test/search_files

check_file "配置文件" "config/search_test_config.json" || failed=1
check_file "关键词文件" "data/search_keywords.json" || failed=1
check_file "公共参数" "../../vds-client/data/public_params.json" || failed=1
check_file "私钥文件" "../../vds-client/data/private_key.dat" || failed=1
check_file "关键词状态" "../../vds-client/data/keyword_states.json" || {
    echo -e "${YELLOW}⚠️  keyword_states.json 不存在 (将由插入测试生成)${NC}"
}
check_dir "客户端数据目录" "../../vds-client/data" || failed=1
check_dir "服务端数据目录" "../../Storage-node/data" || failed=1

cd ../..

echo ""
if [ $failed -eq 0 ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo -e "${GREEN}✅ 所有配置路径验证通过！${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "可以开始运行测试:"
    echo "  cd system_test/insert_files && make run"
    echo "  cd system_test/search_files && make run"
    echo "  cd system_test && ./run_end_to_end_test.sh quick"
    echo ""
else
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo -e "${RED}❌ 配置路径验证失败${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "请检查缺失的文件和目录"
    echo ""
fi

exit $failed
