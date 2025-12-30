#!/bin/bash

# VDS性能测试框架 - 数据清理演示脚本
# 演示三个独立测试的清理逻辑

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "VDS 性能测试框架 - 数据清理演示"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 检查测试可执行文件是否存在
echo "📋 检查测试程序..."
if [ ! -f "insert_files/insert_perf_test" ]; then
    echo "❌ insert_perf_test 不存在，请先编译"
    echo "   cd insert_files && make"
    exit 1
fi

if [ ! -f "search_files/search_perf_test" ]; then
    echo "❌ search_perf_test 不存在，请先编译"
    echo "   cd search_files && make"
    exit 1
fi

if [ ! -f "verify_files/verify_perf_test" ]; then
    echo "❌ verify_perf_test 不存在，请先编译"
    echo "   cd verify_files && make"
    exit 1
fi

echo "✅ 所有测试程序已准备就绪"
echo ""

# 显示当前数据状态
show_data_status() {
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "📊 当前数据状态"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # 数据库文件
    echo ""
    echo "数据库文件:"
    INDEX_DB="../Storage-node/data/index_db.json"
    SEARCH_DB="../Storage-node/data/search_db.json"

    if [ -f "$INDEX_DB" ]; then
        SIZE=$(stat -f%z "$INDEX_DB" 2>/dev/null || stat -c%s "$INDEX_DB" 2>/dev/null || echo "unknown")
        echo "  ✅ index_db.json ($SIZE bytes)"
    else
        echo "  ❌ index_db.json (不存在)"
    fi

    if [ -f "$SEARCH_DB" ]; then
        SIZE=$(stat -f%z "$SEARCH_DB" 2>/dev/null || stat -c%s "$SEARCH_DB" 2>/dev/null || echo "unknown")
        echo "  ✅ search_db.json ($SIZE bytes)"
    else
        echo "  ❌ search_db.json (不存在)"
    fi

    # 客户端文件
    echo ""
    echo "客户端文件:"
    ENC_COUNT=$(find ../vds-client/data/EncFiles -type f 2>/dev/null | wc -l)
    META_COUNT=$(find ../vds-client/data/MetaFiles -type f 2>/dev/null | wc -l)
    INSERT_COUNT=$(find ../vds-client/data/Insert -type f 2>/dev/null | wc -l)
    SEARCH_COUNT=$(find ../vds-client/data/Search -name "*.json" -type f 2>/dev/null | wc -l)

    echo "  EncFiles: $ENC_COUNT 个文件"
    echo "  MetaFiles: $META_COUNT 个文件"
    echo "  Insert: $INSERT_COUNT 个文件"
    echo "  Search: $SEARCH_COUNT 个token文件"

    # 服务端文件
    echo ""
    echo "服务端文件:"
    SERVER_ENC_COUNT=$(find ../Storage-node/data/EncFiles -type f 2>/dev/null | wc -l)
    SERVER_META_COUNT=$(find ../Storage-node/data/metadata -type f 2>/dev/null | wc -l)
    PROOF_COUNT=$(find ../Storage-node/data/SearchProof -name "*.json" -type f 2>/dev/null | wc -l)

    echo "  EncFiles: $SERVER_ENC_COUNT 个文件"
    echo "  metadata: $SERVER_META_COUNT 个文件"
    echo "  SearchProof: $PROOF_COUNT 个证明文件"
    echo ""
}

# 显示初始状态
show_data_status

# 提示用户
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "测试选项"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1. 演示插入测试数据清理"
echo "   - 清理所有数据库和插入相关文件"
echo "   - 重新插入测试文件"
echo ""
echo "2. 演示搜索测试数据清理"
echo "   - 只清理搜索token和proof文件"
echo "   - 保留数据库（依赖插入测试）"
echo ""
echo "3. 演示验证测试数据清理"
echo "   - 不清理任何文件（只读取）"
echo ""
echo "4. 运行完整端到端测试"
echo "   - 依次运行三个测试"
echo ""
echo "5. 手动清理所有数据"
echo ""
echo "6. 退出"
echo ""

read -p "请选择 (1-6): " choice

case $choice in
    1)
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "▶ 运行插入测试（将清理所有数据）"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        cd insert_files
        ./insert_perf_test
        cd ..
        echo ""
        show_data_status
        ;;

    2)
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "▶ 运行搜索测试（只清理搜索文件）"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""

        # 检查数据库是否存在
        if [ ! -f "../Storage-node/data/index_db.json" ] || [ ! -f "../Storage-node/data/search_db.json" ]; then
            echo "⚠️  警告: 数据库不存在！"
            echo "   搜索测试需要插入测试产生的数据库"
            echo "   是否先运行插入测试? (y/n)"
            read -p "> " run_insert
            if [ "$run_insert" = "y" ]; then
                echo ""
                echo "运行插入测试..."
                cd insert_files
                ./insert_perf_test
                cd ..
                echo ""
            else
                echo "跳过搜索测试"
                exit 0
            fi
        fi

        cd search_files
        ./search_perf_test
        cd ..
        echo ""
        show_data_status
        ;;

    3)
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "▶ 运行验证测试（不清理任何文件）"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""

        # 检查证明文件是否存在
        PROOF_COUNT=$(find ../Storage-node/data/SearchProof -name "*.json" -type f 2>/dev/null | wc -l)
        if [ "$PROOF_COUNT" -eq 0 ]; then
            echo "⚠️  警告: 没有找到证明文件！"
            echo "   验证测试需要搜索测试产生的证明文件"
            echo "   是否先运行搜索测试? (y/n)"
            read -p "> " run_search
            if [ "$run_search" = "y" ]; then
                # 先检查数据库
                if [ ! -f "../Storage-node/data/index_db.json" ]; then
                    echo ""
                    echo "数据库不存在，需要先运行插入测试"
                    cd insert_files
                    ./insert_perf_test
                    cd ..
                    echo ""
                fi

                echo ""
                echo "运行搜索测试..."
                cd search_files
                ./search_perf_test
                cd ..
                echo ""
            else
                echo "跳过验证测试"
                exit 0
            fi
        fi

        cd verify_files
        ./verify_perf_test
        cd ..
        echo ""
        show_data_status
        ;;

    4)
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "▶ 运行完整端到端测试"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        ./run_end_to_end_test.sh full
        ;;

    5)
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "⚠️  手动清理所有数据"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "这将删除:"
        echo "  - 所有数据库文件"
        echo "  - 所有客户端文件"
        echo "  - 所有服务端文件"
        echo ""
        read -p "确认删除? (yes/no): " confirm
        if [ "$confirm" = "yes" ]; then
            echo ""
            echo "清理客户端数据..."
            rm -f ../vds-client/data/EncFiles/*
            rm -f ../vds-client/data/MetaFiles/*
            rm -f ../vds-client/data/Insert/*
            rm -f ../vds-client/data/Search/*.json
            rm -f ../vds-client/data/keyword_states.json

            echo "清理服务端数据..."
            rm -f ../Storage-node/data/index_db.json
            rm -f ../Storage-node/data/search_db.json
            rm -f ../Storage-node/data/metadata/*
            rm -f ../Storage-node/data/EncFiles/*
            rm -f ../Storage-node/data/SearchProof/*.json

            echo ""
            echo "✅ 清理完成"
            echo ""
            show_data_status
        else
            echo "取消清理"
        fi
        ;;

    6)
        echo "退出"
        exit 0
        ;;

    *)
        echo "无效选择"
        exit 1
        ;;
esac

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ 完成"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "详细说明请参考: DATA_CLEANUP_GUIDE.md"
echo ""
