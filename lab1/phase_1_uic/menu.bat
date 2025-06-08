@echo off
echo 电子商城系统
echo =======================================
echo 1. 启动命令行版本
echo 2. 启动图形界面版本
echo 3. 安装图形界面依赖
echo 4. 退出
echo =======================================

choice /c 1234 /n /m "请选择: "

if errorlevel 4 goto :exit
if errorlevel 3 goto :install_deps
if errorlevel 2 goto :start_ui
if errorlevel 1 goto :start_cmd

:start_cmd
call start.bat
goto :eof

:start_ui
if exist ui_main.exe (
    call start_ui.bat
) else (
    echo UI版本未编译，请先编译或安装依赖！
    echo 按任意键返回...
    pause > nul
    goto :eof
)
goto :eof

:install_deps
call install_ui_deps.bat
goto :eof

:exit
echo 谢谢使用！
exit /b 0
