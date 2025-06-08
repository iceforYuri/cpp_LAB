# ImGui安装步骤
# 1. 创建ImGui目录
mkdir ./imgui

# 2. 下载ImGui代码
cd imgui
git clone https://github.com/ocornut/imgui .

echo ImGui已下载到imgui目录，接下来会自动下载GLFW库...
pause

# 3. 安装GLFW
# 使用vcpkg或直接下载二进制
# 这里以手动安装为示例：
# 先下载GLFW

# 4. 打开浏览器下载GLFW
start https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip

echo 请下载GLFW并解压到工程目录下的glfw文件夹
pause
