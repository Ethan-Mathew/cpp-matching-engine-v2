#pragma once

#include "RestingOrder.hpp"

#include <cstddef>
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

    template <typename... Args>
    RestingOrder* allocate(Args&&... args);

    void deallocate(RestingOrder* ptr);

    std::size_t get_total_elements() const
    {
        return totalElements_;
    }

    std::size_t get_currently_allocated() const
    {
        return currentlyAllocated_;
    }

    std::size_t get_num_slabs() const
    {
        return slabs_.size();
    }

private:
    union MemoryBlock;

    MemoryBlock* allocate_slab(std::size_t slabSize);

    std::size_t totalElements_      = 0;
    std::size_t currentlyAllocated_ = 0;

    MemoryBlock* firstFree_ = nullptr;

    std::vector<MemoryBlock*> slabs_;
};

union MemoryPool::MemoryBlock
{
    alignas(RestingOrder) std::byte order_[sizeof(RestingOrder)];
    MemoryBlock* next_ = nullptr;
};

template <typename... Args>
RestingOrder* MemoryPool::allocate(Args&&... args)
{
    if (!firstFree_)
    {
        MemoryBlock* newSlab = allocate_slab(totalElements_);
        slabs_.push_back(newSlab);

        firstFree_ = newSlab;

        totalElements_ *= 2;
    }

    MemoryBlock* ret = firstFree_;
    firstFree_ = firstFree_->next_;

    currentlyAllocated_++;

    return new (static_cast<void*>(&ret->order_)) RestingOrder(std::forward<Args>(args)...);
}

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