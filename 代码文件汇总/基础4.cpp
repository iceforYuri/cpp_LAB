#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits> // 用于 std::numeric_limits
using namespace std;

int main()
{
    int num = rand() % 1000 + 1;
    int guess = 0;
    while (guess != num)
    {
        cout << "请输入一个1-1000的整数：" << endl;
        if (!(cin >> guess))
        {
            cout << "输入错误，请输入整数" << endl;
            cin.clear();                                         // 清除错误标志
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 清除输入缓冲区
            continue;                                            // 继续循环，重新输入
        }
        if (guess < 1 || guess > 1000)
        {
            cout << "请输入1-1000范围内的整数！" << endl;
            continue;
        }
        if (guess > num)
        {
            cout << "猜大了" << endl;
        }
        else if (guess < num)
        {
            cout << "猜小了" << endl;
        }
        else
        {
            cout << "猜对了" << endl;
        }
    }
}