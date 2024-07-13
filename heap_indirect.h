#ifndef HEAP_INDIRECT_H_
#define HEAP_INDIRECT_H_
/*
 * Indirect Heap algorithms
 *
 * pretty much a copy of the standard heap algorithms except that they
 * are augmented by storing the element position when it is moved in the
 * heap to fix a heap condition violation.
 *
 * This allows to efficiently implement the delete operation on an
 * arbitrary element instead of reserving the deletion to only the root.
 *
 * Olivier Langlois - July 11, 2024
 */

#include <iterator>

/*
 * uncomment to preserve the heap elements stability. Preserving stability
 * increases the number of operations to fix the heap conditions violation
 * following a removal or a deletion.
 */
//#define BASE_HEAP_PRESERVE_STABILITY 1

namespace Base {

/*
 * the root of the heap is the highest priority element
 * when an element is popped, it is the first element is moved in the last
 * element and the last element is placed in its right place by calling
 * downheap.
 */
namespace HeapHelpers {
template<typename RandomAccessIterator,
         typename Distance,
         typename itemType,
         typename Compare>
constexpr void
upheap(RandomAccessIterator first,
       Distance k,
       Distance topIndex,
       itemType v, Compare & comp)
{
    Distance parent{(k - 1) / 2};

    while (k > topIndex && // sentinel
           comp(*(first + parent), v)) { // if v is greater (if comp is less)
        auto it{first + k};

        // move down the parent
        *(it) = std::move(*(first + parent));
        setHeapIndex(*it, k);
        k = parent;
        parent = (k - 1) / 2;
    }
    auto it{first + k};

    *(it) = std::move(v);
    setHeapIndex(*it, k);
}

template<typename RandomAccessIterator, typename Distance,
         typename itemType, typename Compare>
constexpr void
downheap(RandomAccessIterator first,
         const Distance topIndex, Distance k, Distance len,
         itemType v, Compare comp)
{
    /*
     * initialize secondChild with k to start inspecting its children in the
     * following loop
     */
    Distance secondChild = k;

    // move up the best child
    while (secondChild < (len - 1) / 2) {
        secondChild = 2 * (secondChild + 1);
        // pick the biggest child
        if (comp(*(first + secondChild),
                 *(first + (secondChild - 1))))
            --secondChild;
#ifndef BASE_HEAP_PRESERVE_STABILITY
        if (!comp(v, *(first + secondChild)))
            break;
#endif
        auto it{first + k};

        *(it) = std::move(*(first + secondChild));
        setHeapIndex(*it, k);
        k = secondChild;
    }
#ifndef BASE_HEAP_PRESERVE_STABILITY
    auto it{first + k};

    *(it) = std::move(v);
    setHeapIndex(*it, k);
#else
    /*
     * if heap size is odd
     * and secondchild points on the leaves on the previous generation:
     * ie: len=15, secondChild=6,
     *
     * place the last heap element in the previous generation unique leaf
     * so that the upheap starts on the latest generation.
     */
    if ((len & 1) == 0 &&
        secondChild == (len - 2) / 2) {
        secondChild = 2 * (secondChild + 1);
        const auto secondChildIdx{secondChild - 1};
        auto it{first + k};

        *(it) = std::move(*(first + secondChildIdx));
        setHeapIndex(*it, k);
        k = secondChildIdx;
    }

    /*
     * at this point, k points to the hole, upheap the heap
     * with v from this point.
     *
     * NOTE:
     * This seems like downheap() could be simplified by following
     * the algorithm in Robert Sedgewick book.
     *
     * This needs to be tried once unit tests are in place.
     * this is done to preserve stability.
     */
    upheap(first, k, topIndex, std::move(v), comp);
#endif
}

template<typename RandomAccessIterator, typename Compare>
constexpr inline void
remove(RandomAccessIterator first, RandomAccessIterator last,
       RandomAccessIterator popPos, RandomAccessIterator result,
       Compare& comp)
{
    using ValueType = typename std::iterator_traits<RandomAccessIterator>::value_type;
    using DistanceType = typename std::iterator_traits<RandomAccessIterator>::difference_type;

    /*
     * previous smallest priority element is stored in v
     * to be repositioned.
     */
    ValueType v{std::move(*result)};

    // popPos is going to be popped
    *result = std::move(*popPos);

    downheap(first, DistanceType{},        // topIndex
             DistanceType{popPos - first}, // k
             DistanceType(last - first),   // len
             std::move(v), comp);
}
}

template<typename RandomAccessIterator, typename Compare>
constexpr inline void
upheap(RandomAccessIterator first, RandomAccessIterator last,
       RandomAccessIterator changed, Compare comp)
{
    using ValueType    = typename std::iterator_traits<RandomAccessIterator>::value_type;
    using DistanceType = typename std::iterator_traits<RandomAccessIterator>::difference_type;
    ValueType v = std::move(*changed);

    HeapHelpers::upheap(first,                         // first
                        DistanceType(changed - first), // k
                        DistanceType{},                // top index
                        std::move(v), comp);
}

template<typename RandomAccessIterator, typename Compare>
constexpr inline void
downheap(RandomAccessIterator first, RandomAccessIterator last,
         RandomAccessIterator changed, Compare comp)
{
    using ValueType    = typename std::iterator_traits<RandomAccessIterator>::value_type;
    using DistanceType = typename std::iterator_traits<RandomAccessIterator>::difference_type;
    ValueType v = std::move(*changed);

    HeapHelpers::downheap(first, DistanceType{},         // topIndex
                          DistanceType{changed - first}, // k
                          DistanceType(last - first),    // len
                          std::move(v), comp);
}

/**
 *  @brief  Push an element onto a heap using comparison functor.
 *  @param  first  Start of heap.
 *  @param  last   End of heap + element.
 *  @param  comp   Comparison functor.
 *  @ingroup heap_algorithms
 *
 *  This operation pushes the element at last-1 onto the valid
 *  heap over the range [first,last-1).  After completion,
 *  [first,last) is a valid heap.  Compare operations are
 *  performed using comp.
*/
template<typename RandomAccessIterator, typename Compare>
constexpr inline void
push_heap(RandomAccessIterator first, RandomAccessIterator last,
          Compare comp)
{
    Base::upheap(first, last, last - 1, comp);
}

/**
 *  @brief  Pop an element off a heap using comparison functor.
 *  @param  first  Start of heap.
 *  @param  last   End of heap.
 *  @param  comp   Comparison functor to use.
 *  @ingroup heap_algorithms
 *
 *  This operation pops the top of the heap.  The elements first
 *  and last-1 are swapped and [first,last-1) is made into a
 *  heap.  Comparisons are made using comp.
 */
template<typename RandomAccessIterator, typename Compare>
constexpr inline void
pop_heap(RandomAccessIterator first,
         RandomAccessIterator last, Compare comp)
{
    if (last - first > 1) {
        --last;
        HeapHelpers::remove(first, last, first, last, comp);
    }
}

template<typename RandomAccessIterator, typename Compare>
constexpr inline void
pop_heap(RandomAccessIterator first, RandomAccessIterator last,
         RandomAccessIterator popPos,
         Compare comp)
{
    if (last - first > 1) {
        --last;
        HeapHelpers::remove(first, last, popPos, last, comp);
    }
}
}

#endif
