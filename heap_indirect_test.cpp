/*
 * heap indirect test
 *
 * Olivier Langlois July 12, 2024
 *
 * to compile:
 * g++ -std=c++26 -g heap_indirect_test.cpp
 */

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
#include "heap_indirect.h"

namespace Base {
/*
 * class pointer_iterator
 *
 * iterator proxy that returns the address of the underlying iterator returned
 * value
 *
 * this class is using a C++20 feature:
 * https://en.cppreference.com/w/cpp/language/default_comparisons
 */
template <class iterator>
class pointer_iterator
{
public:
    pointer_iterator(iterator it)
    : m_it(it) {}

    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = typename std::iterator_traits<iterator>::pointer;
    using pointer           = value_type*;
    using reference         = value_type&;

    value_type operator*() const { return &(*m_it); }
    // Prefix increment
    pointer_iterator& operator++() { ++m_it; return *this; }  

    // Postfix increment
    pointer_iterator operator++(int) { auto tmp{*this}; ++(*this); return tmp; }

    friend bool operator== (const pointer_iterator &lhs,
                            const pointer_iterator &rhs) { return lhs.m_it == rhs.m_it; };

private:
    iterator m_it;
};
}

template <typename ValType>
struct TestElem
{
    ValType v;
    uint8_t pos{};
};

/*
 * provide function required by Base::push_heap(), Base::pop_heap()
 */
template <typename ValType>
inline void setHeapIndex(TestElem<ValType> *e, size_t idx)
{
    e->pos = idx;
}

template <typename Iterator>
void printTestVec(Iterator first, Iterator last)
{
    std::for_each(first, last, [](const auto &curItem){
        std::cout << curItem.v << ' ';
    });
    std::cout << '\n';
    std::for_each(first, last, [](const auto &curItem){
        std::cout << static_cast<unsigned>(curItem.pos) << ' ';
    });
    std::cout << '\n';
}

template <typename Iterator>
void printPtrTestVec(Iterator first, Iterator last)
{
    std::for_each(first, last, [](const auto *curItem){
        std::cout << curItem->v << ' ';
    });
    std::cout << '\n';
    std::for_each(first, last, [](const auto *curItem){
        std::cout << static_cast<unsigned>(curItem->pos) << ' ';
    });
    std::cout << '\n';
}

int main(int argc, char *argv[])
{
    auto comp{[](const auto *lhs, const auto *rhs){ return lhs->v > rhs->v; }};
    std::vector<TestElem<int> > intVec{ {1}, {5}, {2}, {6}, {4}, {8}, {7}, {3} };
    std::vector<TestElem<int> *> intPtrVec(Base::pointer_iterator{std::begin(intVec)},
                                           Base::pointer_iterator{std::end(intVec)});
    auto ipit{std::begin(intPtrVec)};
    auto ivit{std::begin(intVec)};
    auto intCmp{[](const TestElem<int> *lhs, const TestElem<int> *rhs){
        return lhs->v < rhs->v;
    }};

    /*
     * test case idea coming from the book:
     * Algorithms in C++ by Robert Sedgewick (1992)
     * Chapter 11 - Priority Queues, Exercise #1
     */
    std::cout << "insert(1),insert(5):\n";
    Base::push_heap(ipit, ipit+2, intCmp);
    printTestVec(ivit, ivit+2);

    std::cout << "\ninsert(2):\n";
    Base::push_heap(ipit, ipit+3, intCmp);
    printTestVec(ivit, ivit+3);

    std::cout << "\ninsert(6):\n";
    Base::push_heap(ipit, ipit+4, intCmp);
    printTestVec(ivit, ivit+4);

    std::cout << "\nreplace(4):\n";
    Base::push_heap(ipit, ipit+5, intCmp);
    printPtrTestVec(ipit, ipit+5);
    Base::pop_heap(ipit, ipit+5, intCmp);
    std::cout << '\n';
    printPtrTestVec(ipit, ipit+4);

    std::cout << "\ninsert(8):\n";
    *(ipit+4) = &intVec[5];
    Base::push_heap(ipit, ipit+5, intCmp);
    printPtrTestVec(ipit, ipit+5);

    std::cout << "\nremove:\n";
    Base::pop_heap(ipit, ipit+5, intCmp);
    printPtrTestVec(ipit, ipit+4);

    std::cout << "\ninsert(7):\n";
    *(ipit+4) = &intVec[6];
    Base::push_heap(ipit, ipit+5, intCmp);
    printPtrTestVec(ipit, ipit+5);

    std::cout << "\ninsert(3):\n";
    *(ipit+5) = &intVec[7];
    Base::push_heap(ipit, ipit+6, intCmp);
    printPtrTestVec(ipit, ipit+6);

    /*
     * test case idea coming from the book:
     * Algorithms in C++ by Robert Sedgewick (1992)
     * Chapter 11 - Priority Queues, Exercise #3
     */
    std::vector<TestElem<char> > charVec{ {'E'}, {'A'}, {'S'}, {'Y'},
                                          {'Q'}, {'U'}, {'E'}, {'S'}, {'T'}, {'I'}, {'O'}, {'N'} };
    std::vector<TestElem<char> *> charPtrVec(Base::pointer_iterator{std::begin(charVec)},
                                        Base::pointer_iterator{std::end(charVec)});
    auto cpit{std::begin(charPtrVec)};
    auto cvit{std::begin(charVec)};
    auto charCmp{[](const TestElem<char> *lhs, const TestElem<char> *rhs){
        return lhs->v < rhs->v;
    }};

    for (size_t offset{2}; offset <= 12; ++offset) {
        std::cout << "\ninsert(" << charVec[offset-1].v << "):\n";
        Base::push_heap(cpit, cpit+offset, charCmp);
        printPtrTestVec(cpit, cpit+offset);
    }
    auto origCharVec{charVec};
    auto origCharPtrVec{charPtrVec};

    /*
     * here we are cheating a bit because we know the element position
     * that we want to pop.
     *
     * The use case that I had in mind is an object that wants to remove
     * itself from the queue, by having its position in the queue, it can
     * do it. (first_iterator+pos)
     *
     * This idea came to me when I wanted to convert an intrusive doubly linked
     * list system to a priority queue.
     *
     * Before the refactoring, all the object had to do to remove itself from
     * the list was to fix its own pointers and the ones from its detaching
     * neighbors.
     *
     * deletion complexity has changed from C to NlogN but the speed gain
     * from the insertions far outweights this loss.
     */
    std::cout << "\nremove E at pos 6:\n";
    Base::pop_heap(cpit, cpit+12, cpit+6, charCmp);
    printPtrTestVec(cpit, cpit+11);

    charVec    = origCharVec;
    charPtrVec = origCharPtrVec;

    std::cout << "\nremove N at pos 5:\n";
    Base::pop_heap(cpit, cpit+12, cpit+5, charCmp);
    printPtrTestVec(cpit, cpit+11);

    charVec    = origCharVec;
    charPtrVec = origCharPtrVec;

    std::cout << "\nremove U at pos 2:\n";
    Base::pop_heap(cpit, cpit+12, cpit+2, charCmp);
    printPtrTestVec(cpit, cpit+11);

    charVec    = origCharVec;
    charPtrVec = origCharPtrVec;

    std::cout << "\nremove T at pos 1:\n";
    Base::pop_heap(cpit, cpit+12, cpit+1, charCmp);
    printPtrTestVec(cpit, cpit+11);

    charVec    = origCharVec;
    charPtrVec = origCharPtrVec;

    std::cout << "\nchange A at pos 7 to V:\n";
    (*(cpit+7))->v = 'V';
    Base::upheap(cpit, cpit+12, cpit+7, charCmp);
    printPtrTestVec(cpit, cpit+12);
    
    return 0;
}
