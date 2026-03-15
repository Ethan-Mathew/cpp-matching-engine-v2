#include "Types/Order.hpp"
#include "Core/MemoryPool.hpp"

#include <iostream>

int main()
{
    Order order;
    MemoryPool pool;

    std::cout << sizeof(Order);
    return 0;
}