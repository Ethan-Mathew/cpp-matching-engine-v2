#pragma once

#include "RestingOrder.hpp"

#include <cstddef>
#include <memory>
#include <new>
#include <vector>

namespace lob::core
{

class MemoryPool
{
public:
    MemoryPool() = delete;
    explicit MemoryPool(std::size_t size);

    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // unsure if I should use parameter pack or just listing the params is fine
    template <typename... Args>
    RestingOrder* allocate(Args&&... args);

    void deallocate(RestingOrder* ptr);


private:
    union MemoryBlock;

    MemoryBlock* allocate_slab(std::size_t slabSize);


    std::size_t totalElements_      = 0;
    std::size_t currentlyAllocated_ = 0;

    MemoryBlock* firstFree_ = nullptr;
    MemoryBlock* pool_      = nullptr;

    std::vector<MemoryBlock*> slabs_;
};

union MemoryPool::MemoryBlock
{
    alignas(RestingOrder) std::byte order_[sizeof(RestingOrder)];
    MemoryBlock* next_ = nullptr;
};

MemoryPool::MemoryBlock* MemoryPool::allocate_slab(std::size_t slabSize)
{
    MemoryBlock* firstInSlab = static_cast<MemoryBlock*>(::operator new((slabSize * sizeof(MemoryBlock)),
                                                         std::align_val_t(alignof(RestingOrder))));

    std::size_t indexOfLastBlock = slabSize - 1;
    for (std::size_t i{}; i < indexOfLastBlock; i++)
    {
        firstInSlab[i].next_ = &firstInSlab[i + 1];
    }

    firstInSlab[indexOfLastBlock].next_ = nullptr;

    return firstInSlab;
}

} // namespace lob::core