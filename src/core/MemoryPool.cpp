#include "lob/Aliases.hpp"

#include "MemoryPool.hpp"
#include "RestingOrder.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>

namespace lob::core
{

MemoryPool::MemoryPool(std::size_t size)
    : totalElements_{size}
{
    MemoryBlock* newSlab = allocate_slab(size);
    slabs_.push_back(newSlab);

    firstFree_ = newSlab;

    totalElements_ += size;
}

MemoryPool::~MemoryPool()
{
    for (MemoryBlock* slab : slabs_)
    {
        ::operator delete(slab, std::align_val_t(alignof(RestingOrder)));
    }
}

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

void MemoryPool::deallocate(RestingOrder* orderToFree)
{
    std::destroy_at(orderToFree);

    auto deallocatedOrder = reinterpret_cast<MemoryBlock*>(orderToFree);

    deallocatedOrder->next_ = firstFree_;
    firstFree_ = deallocatedOrder;

    currentlyAllocated_--;
}

} // namespace lob::core