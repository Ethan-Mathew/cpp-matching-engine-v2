#pragma once

#include "Order.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

class MemoryPool
{
public:
    MemoryPool() = delete;
    explicit MemoryPool(std::size_t size);

    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    template <typename... Args>
    void* allocate(Args... args);


private:
    union MemoryBlock;

    std::uint64_t totalElements_;
    std::uint64_t currentlyAllocated_;

    MemoryBlock* firstFree_;
    MemoryBlock* pool_;
};

union MemoryPool::MemoryBlock
{
    alignas(Order) std::byte order_[sizeof(Order)];
    MemoryBlock* next_;
};
