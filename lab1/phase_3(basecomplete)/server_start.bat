@echo off
REM 设置控制台代码页为UTF-8
chcp 65001
server_oop.exe

REM 如果服务器意外终止，暂停以查看错误信息
echo 服务器已关闭。
pause