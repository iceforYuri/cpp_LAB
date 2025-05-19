# 编译器
CXX = g++
# 编译选项
CXXFLAGS = -std=c++11 -Wall -I.
# 链接选项
LDFLAGS = -lws2_32

# 项目目录
SRC_DIR = f:\Code\20250419(c++lab)\lab1\phase_2
BUILD_DIR = $(SRC_DIR)\build

# 服务器源文件
SERVER_SRCS = $(SRC_DIR)\server\server_main.cpp \
              $(SRC_DIR)\server\request_handler.cpp \
              $(SRC_DIR)\network\socket.cpp \
              $(SRC_DIR)\network\protocol.cpp \
              $(SRC_DIR)\user\user.cpp \
              $(SRC_DIR)\store\store.cpp \
              $(SRC_DIR)\order\order.cpp \
              $(SRC_DIR)\ordermanager\ordermanager.cpp

# 客户端源文件
CLIENT_SRCS = $(SRC_DIR)\client\client_main.cpp \
              $(SRC_DIR)\client\client_api.cpp \
              $(SRC_DIR)\client\client_ui.cpp \
              $(SRC_DIR)\network\socket.cpp \
              $(SRC_DIR)\network\protocol.cpp

# 目标文件
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)\%.cpp=$(BUILD_DIR)\%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)\%.cpp=$(BUILD_DIR)\%.o)

# 目标可执行文件
SERVER_TARGET = $(BUILD_DIR)\server.exe
CLIENT_TARGET = $(BUILD_DIR)\client.exe

# 默认目标
all: create_dirs $(SERVER_TARGET) $(CLIENT_TARGET)

# 创建目录
create_dirs:
	-if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	-if not exist $(BUILD_DIR)\server mkdir $(BUILD_DIR)\server
	-if not exist $(BUILD_DIR)\client mkdir $(BUILD_DIR)\client
	-if not exist $(BUILD_DIR)\network mkdir $(BUILD_DIR)\network
	-if not exist $(BUILD_DIR)\user mkdir $(BUILD_DIR)\user
	-if not exist $(BUILD_DIR)\store mkdir $(BUILD_DIR)\store
	-if not exist $(BUILD_DIR)\order mkdir $(BUILD_DIR)\order
	-if not exist $(BUILD_DIR)\ordermanager mkdir $(BUILD_DIR)\ordermanager
	-if not exist $(BUILD_DIR)\page mkdir $(BUILD_DIR)\page

# 服务器目标
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 客户端目标
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 编译规则
$(BUILD_DIR)\%.o: $(SRC_DIR)\%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# 清理
clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)

.PHONY: all create_dirs clean
