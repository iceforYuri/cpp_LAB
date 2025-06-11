#include <iostream>
#include <cmath>

class shape
{
private:
public:
    shape()
    {
        std::cout << "shape 构造" << std::endl;
    }
    virtual ~shape()
    {
        std::cout << "shape 析构" << std::endl;
    }
    virtual double area() = 0;

    // 重载1：打印信息
    void print()
    {
        std::cout << "这是一个形状" << std::endl;
    }

    // 重载2：打印信息(带信息)
    void print(const std::string &message)
    {
        std::cout << "形状信息: " << message << std::endl;
    }
};

class circle : public shape
{
private:
    double radius;

public:
    circle(double r) : radius(r)
    {
        std::cout << "circle 构造函数被调用，半径=" << radius << std::endl;
    }

    // 析构函数
    ~circle() override
    {
        std::cout << "circle 析构函数被调用，半径=" << radius << std::endl;
    }
    double area() override
    {
        return M_PI * radius * radius;
    }
    double area(int r) const
    {
        return M_PI * r * r;
    }
    // 重写：基类的print方法
    void print()
    {
        std::cout << "这是一个圆形，半径=" << radius << std::endl;
    }
};

class rectangle : public shape
{
private:
    double a;
    double b;

public:
    // 构造函数
    rectangle(double w, double h) : a(w), b(h)
    {
        std::cout << "rectangle 构造函数被调用，宽=" << w << "，高=" << h << std::endl;
    }

    // 析构函数
    virtual ~rectangle() override
    {
        std::cout << "rectangle 析构函数被调用" << std::endl;
    }
    virtual double area() override
    {
        return a * b;
    }
    double area(int x, int y)
    {
        return x * y;
    }
};

class square : public rectangle
{
private:
    double a;

public:
    square(double a) : rectangle(a, a), a(a) // 首先初始化基类，然后初始化自己的成员
    {
        std::cout << "square 构造函数被调用，边长=" << a << std::endl;
    }
    ~square() override
    {
        std::cout << "square 析构函数被调用" << std::endl;
    }
    double area() override
    {
        return a * a;
    }
    double area(int r)
    {
        return r * r;
    }
};

int main()
{
    std::cout << "\n===== 测试1: 单独创建各种形状对象 =====\n"
              << std::endl;

    // 创建圆形对象
    std::cout << "创建圆形对象:" << std::endl;
    circle c(5.0);
    std::cout << "圆形面积: " << c.area() << std::endl;
    c.print();
    std::cout << std::endl;

    // 创建矩形对象
    std::cout << "创建矩形对象:" << std::endl;
    rectangle r(4.0, 6.0);
    std::cout << "矩形面积: " << r.area() << std::endl;
    r.print();
    std::cout << std::endl;

    // 创建正方形对象
    std::cout << "创建正方形对象:" << std::endl;
    square s(3.0);
    std::cout << "正方形面积: " << s.area() << std::endl;
    s.print();
    std::cout << std::endl;

    std::cout << "\n===== 测试2: 观察对象生命周期 =====\n"
              << std::endl;

    std::cout << "创建局部作用域..." << std::endl;
    {
        std::cout << "进入局部作用域" << std::endl;
        circle localCircle(2.0);
        rectangle localRect(2.0, 3.0);
        square localSquare(4.0);
        std::cout << "即将离开局部作用域" << std::endl;
    } // 局部对象在这里销毁
    std::cout << "已离开局部作用域，局部对象已被销毁" << std::endl;

    std::cout << "\n===== 测试3: 使用基类指针实现多态 =====\n"
              << std::endl;

    // 使用基类指针数组存储不同类型的对象
    shape *shapes[3];
    std::cout << "创建形状数组:" << std::endl;
    shapes[0] = new circle(3.0);
    shapes[1] = new rectangle(5.0, 4.0);
    shapes[2] = new square(2.0);

    // 通过基类指针调用虚函数
    std::cout << "\n计算不同形状的面积:" << std::endl;
    for (int i = 0; i < 3; i++)
    {
        std::cout << "形状 " << (i + 1) << " 的面积: " << shapes[i]->area() << std::endl;
        shapes[i]->print(); // 多态调用
    }

    // 释放动态分配的对象
    std::cout << "\n释放动态创建的对象:" << std::endl;
    for (int i = 0; i < 3; i++)
    {
        delete shapes[i];
    }

    std::cout << "\n===== 测试4: 重载方法测试 =====\n"
              << std::endl;

    // 测试重载的area方法
    std::cout << "调用重载的area方法:" << std::endl;
    std::cout << "圆形: 指定半径为4的圆面积 = " << c.area(4) << std::endl;
    std::cout << "矩形: 指定宽3高7的矩形面积 = " << r.area(3, 7) << std::endl;
    std::cout << "正方形: 指定边长5的正方形面积 = " << s.area(5) << std::endl;

    // 测试重载的print方法
    std::cout << "\n调用重载的print方法:" << std::endl;
    c.print();
    r.print("这是一个矩形对象");
    s.print("这是一个正方形对象");

    std::cout << "\n===== 程序结束, 剩余对象将被销毁 =====\n"
              << std::endl;

    // 使用system("pause")暂停程序(仅适用于Windows)
    system("pause");
    return 0;
}