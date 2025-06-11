#include <iostream>

class matrix
{
private:
    int **matri;
    int rows;
    int cols;

public:
    matrix(int rows, int cols) // 创建一个rows行lines列的矩阵
    {
        this->rows = rows;
        this->cols = cols;
        matri = new int *[rows];
        for (int i = 0; i < rows; i++)
        {
            matri[i] = new int[cols];
        }
        for (int i = 0; i < rows; i++) // 默认初始化
        {
            for (int j = 0; j < cols; j++)
            {
                matri[i][j] = 0;
            }
        }
    }
    matrix()
    {
        std::cout << "请输入行数和列数：" << std::endl;
        std::cin >> rows >> cols;
        matri = new int *[rows];
        for (int i = 0; i < rows; i++)
        {
            matri[i] = new int[cols];
        }
        std::cout << "请输入矩阵元素：" << std::endl;
        matrixinput();
    }
    matrix(const matrix &m) // 拷贝构造
    {
        this->rows = m.rows;
        this->cols = m.cols;
        matri = new int *[rows];
        for (int i = 0; i < rows; i++)
        {
            matri[i] = new int[cols];
        }
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                matri[i][j] = m.matri[i][j];
            }
        }
    }

    ~matrix() // 析构函数，释放内存
    {
        if (matri != nullptr)
        {
            for (int i = 0; i < rows; i++)
            {
                if (matri[i] != nullptr)
                {
                    delete[] matri[i];
                    matri[i] = nullptr;
                }
            }
            delete[] matri;
            matri = nullptr;
        }
    }

    void matrixinput()
    {
        for (int i = 0; i < this->rows; i++)
        {
            for (int j = 0; j < this->cols; j++)
            {
                std::cin >> matri[i][j];
            }
        }
    }

    void matrixoutput()
    {
        for (int i = 0; i < this->rows; i++)
        {
            for (int j = 0; j < this->cols; j++)
            {
                std::cout << matri[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }
    // 友元函数方法可以需要两个参数（左右操作数）
    // 也可以放置在类外部，作为全局函数
    friend matrix operator+(const matrix &m1, const matrix &m2)
    {
        if (m1.rows != m2.rows || m1.cols != m2.cols)
        {
            std::cout << "矩阵大小不同，无法相加" << std::endl;
            return matrix(m1);
        }
        matrix m3(m1.rows, m1.cols);
        for (int i = 0; i < m1.rows; i++)
        {
            for (int j = 0; j < m1.cols; j++)
            {
                m3.matri[i][j] = m1.matri[i][j] + m2.matri[i][j];
            }
        }
        return m3;
    }

    matrix operator-(const matrix &m1)
    {
        if (rows != m1.rows || cols != m1.cols)
        {
            std::cout << "矩阵大小不同，无法相减" << std::endl;
            return matrix(*this);
        }
        matrix m3(rows, cols);
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                m3.matri[i][j] = matri[i][j] - m1.matri[i][j];
            }
        }
        return m3;
    }

    void matrixplus(const matrix &m1, const matrix &m2)
    {
        if (m1.rows != m2.rows || m1.cols != m2.cols)
        {
            std::cout << "矩阵大小不同，无法相加" << std::endl;
            return;
        }
        for (int i = 0; i < m1.rows; i++)
        {
            for (int j = 0; j < m1.cols; j++)
            {
                matri[i][j] = m1.matri[i][j] + m2.matri[i][j];
            }
        }
    }
 
    void matrixminus(const matrix &m1, const matrix &m2)
    {
        if (m1.rows != m2.rows || m1.cols != m2.cols)
        {
            std::cout << "矩阵大小不同，无法相减" << std::endl;
            return;
        }
        for (int i = 0; i < m1.rows; i++)
        {
            for (int j = 0; j < m1.cols; j++)
            {
                matri[i][j] = m1.matri[i][j] - m2.matri[i][j];
            }
        }
    }

    matrix &operator=(const matrix &m) // 复制并交换？
    {
        if (this == &m) // 自赋值检查
        {
            return *this;
        }
        if (matri != nullptr) // 旧内存释放
        {
            for (int i = 0; i < rows; i++)
            {
                if (matri[i] != nullptr)
                {
                    delete[] matri[i];
                    matri[i] = nullptr;
                }
            }
            delete[] matri;
            matri = nullptr;
        }

        rows = m.rows;
        cols = m.cols;
        matri = new int *[rows];
        for (int i = 0; i < rows; i++)
        {
            matri[i] = new int[cols];
            for (int j = 0; j < cols; j++)
            {
                matri[i][j] = m.matri[i][j];
            }
        }
        return *this;
    }
};

int main()
{
    matrix A1 = matrix();
    matrix A2 = matrix();

    matrix A3 = A1 + A2;
    std::cout << "相加后的矩阵：" << std::endl;
    A3.matrixoutput();
    matrix A4 = matrix(A3);
    std::cout << "拷贝构造相加的矩阵：" << std::endl;
    A4.matrixoutput();

    int rows, cols;
    std::cout << "请输入行数和列数：" << std::endl;
    std::cin >> rows >> cols;
    matrix B1(rows, cols);
    matrix B2(rows, cols);
    std::cout << "请输入第一个矩阵：" << std::endl;
    B1.matrixinput();
    std::cout << "请输入第二个矩阵：" << std::endl;
    B2.matrixinput();
    matrix B3 = B1 + B2;
    std::cout << "相加后的矩阵：" << std::endl;
    B3.matrixoutput();
    matrix B4 = B1 - B2;
    std::cout << "相减后的矩阵：" << std::endl;
    B4.matrixoutput();

    matrix *C1 = new matrix(2, 2);
    matrix *C2 = new matrix(2, 2);
    std::cout << "请输入第一个矩阵：" << std::endl;
    C1->matrixinput();
    std::cout << "请输入第二个矩阵：" << std::endl;
    C2->matrixinput();
    matrix C3 = *C1 + *C2;
    std::cout << "相加后的矩阵：" << std::endl;
    C3.matrixoutput();
    matrix C4 = *C1 - *C2;
    std::cout << "相减后的矩阵：" << std::endl;
    C4.matrixoutput();
    delete C1;
    delete C2;

    return 0;
}