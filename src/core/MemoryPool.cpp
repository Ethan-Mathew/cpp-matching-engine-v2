#include "lob/Aliases.hpp"

#include "MemoryPool.hpp"
#include "RestingOrder.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace lob::core
{

MemoryPool::MemoryPool(std::size_t size)
    : totalElements_{size}
{
    MemoryBlock* newSlab = MemoryPool::allocate_slab(size);
    slabs_.push_back(newSlab);

    firstFree_ = newSlab;
}

MemoryPool::~MemoryPool()
{
    for (MemoryBlock* slab : slabs_)
    {
        ::operator delete(slab, std::align_val_t(alignof(RestingOrder)));
    }
}

void MemoryPool::deallocate(RestingOrder* orderToFree)
{
    assert(orderToFree != nullptr);

    std::destroy_at(orderToFree);

    auto deallocatedOrder = reinterpret_cast<MemoryBlock*>(orderToFree);

    deallocatedOrder->next_ = firstFree_;
    firstFree_ = deallocatedOrder;

    currentlyAllocated_--;
}

} // namespace lob::core