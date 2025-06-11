#include <iostream>
#include <cmath>
#include <iomanip>
using namespace std;
class Point
{
private:
    int x;
    int y;

public:
    Point(int x = 0, int y = 0)
    {

        this->x = x;
        this->y = y;
        cout << "构造点函数" << endl;
    }
    ~Point()
    {
        cout << "析构点函数，位置：（" << x << "，" << y << "）" << endl;
    }
    // 输入方法
    void input()
    {
        std::cout << "请输入x坐标: ";
        std::cin >> x;
        std::cout << "请输入y坐标: ";
        std::cin >> y;
    }

    // 静态工厂方法
    static Point createFromInput()
    {
        Point p;
        p.input();
        return p;
    }

    // 获取坐标
    int getX() const
    {
        return x;
    }
    int getY() const
    {
        return y;
    }

    double distance(const Point &p)
    {
        return sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y));
    }
};
class Circle
{
private:
    Point center;
    int radius;

public:
    Circle(const Point &center)
    {
        this->center = center;
        std::cout << "请输入半径：";
        std::cin >> radius;
    }

    // 默认构造，通过输入创建
    Circle()
    {
        std::cout << "是否要输入圆心坐标？(y/n): ";
        char choice;
        std::cin >> choice;

        if (choice == 'y' || choice == 'Y')
        {
            int x, y;
            std::cout << "请输入x坐标: ";
            std::cin >> x;
            std::cout << "请输入y坐标: ";
            std::cin >> y;
            center = Point(x, y);
        }
        else
        {
            center = Point();
            std::cout << "使用默认圆心坐标(0,0)" << std::endl;
        }

        std::cout << "请输入半径：";
        std::cin >> radius;
    }
    ~Circle()
    {
        std::cout << "析构圆函数，圆心：（" << center.getX() << "，" << center.getY()
                  << "），半径：" << radius << std::endl;
    }
    bool isIntersect(const Circle &c)
    {
        return center.distance(c.center) < radius + c.radius; // 不包括相切
    }
};

int main()
{
    std::cout << "创建第一个点：" << std::endl;
    // 使用静态工厂方法创建点
    Point p1 = Point::createFromInput();
    std::cout << "创建的点坐标为: (" << p1.getX() << ", " << p1.getY() << ")" << std::endl;

    std::cout << "创建第二个点：" << std::endl;

    Point p2;
    p2.input();
    std::cout << "创建的点坐标为: (" << p2.getX() << ", " << p2.getY() << ")" << std::endl;

    std::cout << "使用第一个点创建圆：" << std::endl;
    Circle c1(p1);

    std::cout << "直接创建圆：" << std::endl;
    Circle c2;

    if (c1.isIntersect(c2))
    {
        std::cout << "两个圆相交" << std::endl;
    }
    else
    {
        std::cout << "两个圆不相交" << std::endl;
    }

    return 0;
}