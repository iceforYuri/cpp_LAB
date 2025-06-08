# 电子商城系统 - UI安装指南

这个文档将帮助您为电子商城系统安装图形用户界面(UI)版本所需的依赖项。

## 安装步骤

### 1. 安装MinGW-64位(如果尚未安装)
确保您已经安装了支持C++11或更高版本的MinGW:
- 访问 http://mingw-w64.org/doku.php/download 下载并安装

### 2. 安装依赖项

#### 方法1: 使用自动安装脚本
运行项目根目录下的 `install_ui_deps.bat` 脚本，它将自动下载和设置:
- Dear ImGui
- GLFW库

#### 方法2: 手动安装

1. **安装ImGui**:
   - 创建一个 `imgui` 目录在项目根目录下
   - 下载ImGui: https://github.com/ocornut/imgui
   - 将ImGui的所有文件解压到 `imgui` 目录

2. **安装GLFW**:
   - 下载GLFW: https://www.glfw.org/download.html (选择Windows pre-compiled binaries)
   - 创建 `glfw` 目录在项目根目录下
   - 将GLFW解压缩，将 `include` 和 `lib-mingw-w64` 文件夹复制到 `glfw` 目录

### 3. 项目结构

安装完成后，项目目录结构应如下所示:
```
project/
  ├── imgui/            # ImGui库文件
  ├── glfw/             # GLFW库文件
  │   ├── include/      # GLFW头文件
  │   └── lib-mingw-w64/# GLFW库文件
  ├── ui/               # UI代码
  ├── user/             # 用户管理代码  
  ├── store/            # 商店管理代码
  ├── page/             # 页面管理代码
  ├── ui_main.cpp       # UI版本入口
  └── start.cpp         # 命令行版本入口
```

### 4. 编译

#### 使用VS Code:
1. 打开VS Code中的项目
2. 按下 `Ctrl+Shift+B` 选择编译任务
3. 选择 "C/C++: g++.exe 编译UI版本" 编译UI版本

#### 使用命令行:
```
g++ ui_main.cpp ui/ui.cpp user/user.cpp store/store.cpp imgui/*.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp -Iglfw/include -Lglfw/lib-mingw-w64 -lglfw3 -lopengl32 -lgdi32 -o ui_main.exe
```

### 5. 运行

编译成功后，运行 `menu.bat` 选择UI版本或命令行版本启动系统。

## 故障排除

### 编译错误:
- 确保已正确安装所有依赖项
- 确保MinGW已添加到系统路径中
- 确保项目结构正确

### 运行错误:
- 如果出现缺少DLL错误，确保GLFW的DLL文件与可执行文件在同一目录


## 更多资源
- ImGui文档: https://github.com/ocornut/imgui/wiki
- GLFW文档: https://www.glfw.org/documentation.html
