#include <assert.h>
#include <iostream>
#include "allocator.cpp"


void test_basic_allocation_firstFit() {
    auto p1 = alloc(3);
    auto p1b = getHeader(p1);
    assert(p1b->size == sizeof(word_t));

    auto p2 = alloc(8);
    auto p2b = getHeader(p2);
    assert(p2b->size == 8);

    free(p2);
    assert(p2b->used == false);

    auto p3 = alloc(8);
    auto p3b = getHeader(p3);
    assert(p3b->size == 8);
    assert(p3b == p2b);

    std::cout << "✅ test_basic_allocation passed!\n";
    resetHeap();
}


void test_nextFit_allocation_one() {
    init(SearchMode::NextFit);
    auto o1 = alloc(8);
    auto o2 = alloc(8);
    auto o3 = alloc(8);
    
    // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
    auto o4 = alloc(16);
    auto o5 = alloc(16);
    
    // [[8, 1], [8, 1], [8, 1], [16, 0], [16, 0]]
    free(o4);
    free(o5);
    
    // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
    auto o6 = alloc(16);
    
    // Start position from o6:
    assert(searchStart == getHeader(o6));
    // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
    //                           ^ start here
    
    alloc(16);
    std::cout << "✅ test_nextFit_allocation_one passed!\n";
    resetHeap();
}

void test_nextFit_allocation_two(){
    init(SearchMode::NextFit);

    auto a1 = alloc(8);
    auto a2 = alloc(8);
    auto a3 = alloc(8);

    auto a4 = alloc(16);
    auto a5 = alloc(16);

    free(a4);
    free(a5);

    // Allocate a6 again, should go into a4's slot
    auto a6 = alloc(16);
    assert(getHeader(a6) == getHeader(a4));
    assert(searchStart == getHeader(a6));
    // Test wrap-around behavior
    auto a7 = alloc(16); // This should go into a5
    assert(getHeader(a7) == getHeader(a5));
    assert(searchStart == getHeader(a7));

    // Now the heap has no free 16-byte blocks
    // Allocate a larger block to force sbrk (new memory)
    auto a8 = alloc(32);
    assert(getHeader(a8)->size == align(32));
    assert(getHeader(a8)->used == true);

    // // Check that the searchStart is not pointing to a reused free block
    auto a9 = alloc(1000);
    assert(getHeader(a9)->used == true);

    std::cout << "✅ test_nextFit_allocation_two passed!\n";
    resetHeap();
}


void test_bestFit_allocation(){
    init(SearchMode::BestFit);
    
    // [[8, 1], [64, 1], [8, 1], [16, 1]]
    auto a1 = alloc(8);
    auto z1 = alloc(64);
    auto a2 = alloc(8);
    auto z2 = alloc(16);
    
    // Free the last 16
    free(z2);
    
    // Free 64:
    free(z1);
    
    // [[8, 1], [64, 0], [8, 1], [16, 0]]
    
    // Reuse the last 16 block:
    auto z3 = alloc(16);
    assert(getHeader(z3) == getHeader(z2));
    
    // // [[8, 1], [64, 0], [8, 1], [16, 1]]
    
    z3 = alloc(16);
    // // Reuse 64, splitting it to 16, and 24 (considering header)

    // [[8, 1], [16, 1], [24, 0], [8, 1], [16, 1]]
    assert(getHeader(z3) == getHeader(z1));
    
    auto z4 = alloc(24);

    assert(getHeader(z3)->next == getHeader(z4));
    // [[8, 1], [16, 1], [24, 1], [8, 1], [16, 1]]
    //             z3     ^ z4
    //              '     '
    //              '-----'
    //               next  
    assert(getHeader(z4)->next == getHeader(a2));
    std::cout << "✅ test_bestFit_allocation passed!\n";
    resetHeap();
}


int main(int argc, char const *argv[]) {
    test_basic_allocation_firstFit();
    test_nextFit_allocation_one();
    test_nextFit_allocation_two();
    test_bestFit_allocation();

    std::cout << "\nAll tests passed!\n";
    return 0;
}