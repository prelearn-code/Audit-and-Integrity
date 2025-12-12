#include<iostream>

void test1()
{
    for(int i = 0; i < 10; ++i)
    {
        std::cout << "Test1 - Iteration: " << i << std::endl;
    }   
}

int main() {
    test1();
    return 0;
}