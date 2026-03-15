#pragma once

#include "Order.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

union MemoryBlock
{
    alignas(Order) std::byte order_[sizeof(Order)];
    MemoryBlock* next_;
};

static_assert(sizeof(MemoryBlock) == 64);
static_assert(alignof(MemoryBlock) == 64);

class MemoryPool
{
public:
    MemoryPool() = delete;
    explicit MemoryPool(std::size_t size);

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    template <typename... Args>
    void* allocate(Args... args);


private:
    //class MemoryBlock;

    std::uint64_t totalElements_;
    std::uint64_t currentlyAllocated_;
    MemoryBlock* firstFree_;
};