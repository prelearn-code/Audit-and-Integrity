# ============================================================
# Makefile for VDS Search Performance Test
# ============================================================

# 编译器配置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2

# 目录配置
PROJECT_ROOT = ../..
CLIENT_DIR = $(PROJECT_ROOT)/vds-client
SERVER_DIR = $(PROJECT_ROOT)/Storage-node
TEST_DIR = .

# 包含路径
INCLUDES = -I$(CLIENT_DIR) -I$(SERVER_DIR) -I/usr/local/include -I/usr/include/jsoncpp

# 库路径和链接库
LIBS = -L/usr/local/lib -lpbc -lgmp -lcrypto -ljsoncpp -lstdc++fs

# 源文件
SOURCES = search_test.cpp \
          $(CLIENT_DIR)/client.cpp \
          $(SERVER_DIR)/storage_node.cpp

# 目标文件
TARGET = search_perf_test

# 结果目录
RESULTS_DIR = results

# ============================================================
# 构建目标
# ============================================================

.PHONY: all clean clean-data run help setup

# 默认目标
all: setup $(TARGET)

# 编译主程序
$(TARGET): $(SOURCES)
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "🔨 编译搜索性能测试程序..."
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LIBS)
	@echo "✅ 编译完成: $(TARGET)"
	@echo ""

# 创建必要的目录
setup:
	@mkdir -p $(RESULTS_DIR)
	@echo "✅ 结果目录已准备: $(RESULTS_DIR)"

# 清理编译文件
clean:
	@echo "🧹 清理编译文件..."
	@rm -f $(TARGET)
	@echo "✅ 清理完成"

# 清理所有（包括结果）
clean-all: clean
	@echo "🧹 清理所有文件（包括结果）..."
	@rm -rf $(RESULTS_DIR)
	@echo "✅ 完全清理完成"

# 清理测试产生的数据
clean-data:
	@echo "🧹 清理搜索测试产生的数据..."
	@echo "  - 清理客户端搜索Token文件 (../../vds-client/data/Search/search_*.json)"
	@rm -f ../../vds-client/data/Search/search_*.json 2>/dev/null || true
	@echo "  - 清理服务端搜索证明文件 (../../Storage-node/data/SearchProof/proof_*.json)"
	@rm -f ../../Storage-node/data/SearchProof/proof_*.json 2>/dev/null || true
	@echo "✅ 搜索数据清理完成"

# 运行测试
run: $(TARGET)
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "▶️  运行搜索性能测试..."
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	./$(TARGET)

# 使用自定义配置运行
run-config: $(TARGET)
	@if [ -z "$(CONFIG)" ]; then \
		echo "❌ 错误: 请指定配置文件"; \
		echo "用法: make run-config CONFIG=your_config.json"; \
		exit 1; \
	fi
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "▶️  运行搜索性能测试 (配置: $(CONFIG))..."
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	./$(TARGET) $(CONFIG)

# 查看结果
show-results:
	@if [ -f "$(RESULTS_DIR)/search_summary.json" ]; then \
		echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
		echo "🔍 测试结果总结"; \
		echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
		cat $(RESULTS_DIR)/search_summary.json | jq '.' || cat $(RESULTS_DIR)/search_summary.json; \
	else \
		echo "❌ 未找到结果文件: $(RESULTS_DIR)/search_summary.json"; \
		echo "请先运行: make run"; \
	fi

# 帮助信息
help:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "🔍 VDS 搜索性能测试 - Makefile 帮助"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "可用目标:"
	@echo "  make              - 编译程序（默认）"
	@echo "  make run          - 编译并运行测试（使用默认配置）"
	@echo "  make run-config   - 使用自定义配置运行"
	@echo "                      示例: make run-config CONFIG=my.json"
	@echo "  make show-results - 查看测试结果"
	@echo "  make clean        - 清理编译文件"
	@echo "  make clean-all    - 清理所有文件（包括结果）"
	@echo "  make help         - 显示此帮助信息"
	@echo ""
	@echo "配置文件:"
	@echo "  默认: config/search_test_config.json"
	@echo ""
	@echo "结果文件:"
	@echo "  CSV:  $(RESULTS_DIR)/search_detailed.csv"
	@echo "  JSON: $(RESULTS_DIR)/search_summary.json"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
