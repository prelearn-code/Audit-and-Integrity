#!/bin/bash

# ============================================================
# 安装 VDS 性能测试所需的依赖库
# ============================================================

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 VDS 性能测试依赖安装脚本"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 检查是否为 root
if [ "$EUID" -ne 0 ]; then
    echo "❌ 错误: 此脚本需要 root 权限"
    echo "请使用: sudo $0"
    exit 1
fi

echo "🔍 检测系统类型..."
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VER=$VERSION_ID
    echo "✅ 检测到: $PRETTY_NAME"
else
    echo "❌ 无法检测系统类型"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📥 安装依赖包"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

case "$OS" in
    ubuntu|debian)
        echo "▶ 更新软件包列表..."
        apt-get update

        echo ""
        echo "▶ 安装编译工具..."
        apt-get install -y build-essential

        echo ""
        echo "▶ 安装 PBC 库..."
        apt-get install -y libpbc-dev || {
            echo "⚠️  libpbc-dev 在标准源中不可用，尝试手动安装..."

            # 检查是否已经有 PBC 源码
            if [ ! -d "/tmp/pbc-0.5.14" ]; then
                echo "  → 下载 PBC 0.5.14..."
                cd /tmp
                wget https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz
                tar -xzf pbc-0.5.14.tar.gz
            fi

            cd /tmp/pbc-0.5.14
            echo "  → 配置..."
            ./configure
            echo "  → 编译..."
            make
            echo "  → 安装..."
            make install
            ldconfig

            echo "✅ PBC 库手动安装完成"
        }

        echo ""
        echo "▶ 安装 GMP 库..."
        apt-get install -y libgmp-dev

        echo ""
        echo "▶ 安装 OpenSSL..."
        apt-get install -y libssl-dev

        echo ""
        echo "▶ 安装 JsonCpp..."
        apt-get install -y libjsoncpp-dev

        ;;

    centos|rhel|fedora)
        echo "▶ 安装编译工具..."
        if command -v dnf &> /dev/null; then
            dnf groupinstall -y "Development Tools"
            dnf install -y gmp-devel openssl-devel jsoncpp-devel
        else
            yum groupinstall -y "Development Tools"
            yum install -y gmp-devel openssl-devel jsoncpp-devel
        fi

        echo ""
        echo "⚠️  PBC 库需要手动安装..."
        echo "  请访问: https://crypto.stanford.edu/pbc/"
        ;;

    *)
        echo "❌ 不支持的系统: $OS"
        echo "请手动安装以下依赖:"
        echo "  - build-essential / gcc / g++"
        echo "  - libpbc-dev"
        echo "  - libgmp-dev"
        echo "  - libssl-dev"
        echo "  - libjsoncpp-dev"
        exit 1
        ;;
esac

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔍 验证安装"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 验证编译器
if command -v g++ &> /dev/null; then
    echo "✅ g++ 编译器: $(g++ --version | head -1)"
else
    echo "❌ g++ 未找到"
fi

# 验证库文件
echo ""
echo "验证库文件..."

check_lib() {
    local lib_name=$1
    local header_file=$2

    if [ -f "$header_file" ] || [ -d "$header_file" ]; then
        echo "  ✅ $lib_name: $header_file"
        return 0
    else
        # 尝试搜索
        local found=$(find /usr/include /usr/local/include -name "$(basename $header_file)" 2>/dev/null | head -1)
        if [ -n "$found" ]; then
            echo "  ✅ $lib_name: $found"
            return 0
        else
            echo "  ❌ $lib_name: 未找到"
            return 1
        fi
    fi
}

failed=0

check_lib "PBC" "/usr/include/pbc/pbc.h" || check_lib "PBC" "/usr/local/include/pbc/pbc.h" || failed=1
check_lib "GMP" "/usr/include/gmp.h" || check_lib "GMP" "/usr/local/include/gmp.h" || failed=1
check_lib "OpenSSL" "/usr/include/openssl/ssl.h" || failed=1
check_lib "JsonCpp" "/usr/include/json/json.h" || check_lib "JsonCpp" "/usr/include/jsoncpp/json/json.h" || failed=1

echo ""
if [ $failed -eq 0 ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "✅ 所有依赖安装成功！"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "现在可以编译测试程序:"
    echo "  cd system_test/insert_files && make"
    echo "  cd system_test/search_files && make"
    echo ""
else
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "⚠️  部分依赖安装失败"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "请检查上面的错误信息并手动安装缺失的库"
    echo ""
fi

exit $failed
