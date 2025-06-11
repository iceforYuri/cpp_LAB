#include <iostream>

class matrix
{
private:
    int **matri;
    int row;
    int col;

public:
    matrix(int row, int col) // 创建一个row行col列的矩阵
    {
        this->row = row;
        this->col = col;
        matri = new int *[row];
        for (int i = 0; i < row; i++)
        {
            matri[i] = new int[col];
        }
    }

    ~matrix()
    {
        if (matri != nullptr)
        {
            for (int i = 0; i < row; i++)
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
        for (int i = 0; i < this->row; i++)
        {
            for (int j = 0; j < this->col; j++)
            {
                std::cin >> matri[i][j];
            }
        }
    }

    void matrixoutput()
    {
        for (int i = 0; i < this->row; i++)
        {
            for (int j = 0; j < this->col; j++)
            {
                std::cout << matri[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }
    // 使用常量引用而杜绝拷贝构造函数的调用
    void matrixplus(const matrix &m1, const matrix &m2)
    {
        if (m1.row != m2.row || m1.col != m2.col)
        {
            std::cout << "矩阵大小不同，无法相加" << std::endl;
            return;
        }
        for (int i = 0; i < m1.row; i++)
        {
            for (int j = 0; j < m1.col; j++)
            {
                matri[i][j] = m1.matri[i][j] + m2.matri[i][j];
            }
        }
    }

    void matrixminus(const matrix &m1, const matrix &m2)
    {
        if (m1.row != m2.row || m1.col != m2.col)
        {
            std::cout << "矩阵大小不同，无法相减" << std::endl;
            return;
        }
        for (int i = 0; i < m1.row; i++)
        {
            for (int j = 0; j < m1.col; j++)
            {
                matri[i][j] = m1.matri[i][j] - m2.matri[i][j];
            }
        }
    }
};

int main()
{
    matrix m(4, 5);
    std::cout << "矩阵创建成功，请输入4*5矩阵" << std::endl;
    m.matrixinput();
    std::cout << "矩阵为：" << std::endl;
    m.matrixoutput();
    std::cout << "请输入两个矩阵，用于相加和相减" << std::endl;
    matrix m1(4, 5);
    m1.matrixinput();
    matrix m2(4, 5);
    m2.matrixinput();
    m2.matrixoutput();
    matrix m3(4, 5);
    m3.matrixminus(m, m1);
    m3.matrixoutput();
    m3.matrixplus(m, m1);
    m3.matrixoutput();
    // m1.~matrix();
    // m2.~matrix();
    // m3.~matrix();

    int row, col;
    std::cout << "请输入行数和列数：" << std::endl;
    std::cin >> row >> col;
    matrix *A1 = new matrix(row, col);
    matrix *A2 = new matrix(row, col);
    matrix *A3 = new matrix(row, col);
    std::cout << "请输入两个矩阵，用于相加和相减" << std::endl;
    A1->matrixinput();
    A2->matrixinput();
    A3->matrixminus(*A1, *A2);
    A3->matrixoutput();
    A3->matrixplus(*A1, *A2);
    A3->matrixoutput();
    delete A1;
    delete A2;
    delete A3;
    // A1.~matrix();
    // A2.~matrix();
    // A3.~matrix();
    return 0;
}