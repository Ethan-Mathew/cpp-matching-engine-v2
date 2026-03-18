#include "Aliases.hpp"
#include "Order.hpp"
#include "MemoryPool.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
//#include <mutex>
#include <new>
//#include <thread>
//#include <utility>

explicit MemoryPool::MemoryPool(std::size_t size)
    : totalElements_{size}
{
    pool_ = static_cast<MemoryBlock*>(::operator new((size * sizeof(MemoryBlock)),
                                                           std::align_val_t(alignof(Order))));

    std::size_t indexOfLastBlock = size - 1;
    for (std::size_t i{}; i < indexOfLastBlock; i++)
    {
        pool_[i].next_ = &pool_[i + 1];
    }

    pool_[indexOfLastBlock].next_ = nullptr;
    firstFree_ = pool_;                          
}

MemoryPool::~MemoryPool()
{
    ::operator delete(pool_);
}

template <typename... Args>
Order* allocate(Args... args)
{
    if (!firstFree)
    {
        std::thread allocateMoreMemory{};
        allocateMoreMemory.detach();
    }

    MemoryBlock ret = firstFree_;
    firstFree_ = firstFree_->next_;

    return new (static_cast<void*>(&ret->order_)) Order(args...);
}

void* deallocate(Order* orderToFree)
{
    
}


