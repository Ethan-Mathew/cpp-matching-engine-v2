#include "Aliases.hpp"
#include "Order.hpp"
#include "MemoryPool.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>

explicit MemoryPool::MemoryPool(std::size_t size)
    : totalElements_{size}
{
    firstFree_ = static_cast<MemoryBlock*>(::operator new((size * sizeof(MemoryBlock)),
                                                           std::align_val_t(alignof(Order))));

    std::size_t indexOfLastBlock = size - 1;
    for (std::size_t i{}; i < indexOfLastBlock; i++)
    {
        firstFree_[i].next_ = &firstFree_[i + 1];
    }

    firstFree_[indexOfLastBlock].next_ = nullptr;                            
}

template <typename... Args>
void* allocate(Args... args)
{
    firstFree->
    firstFree_ = new () (args...)
}






    //static_assert(sizeof(MemoryBlock) == 64);
