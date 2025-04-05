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


void test_split_block_bestFit(){
    init(SearchMode::BestFit);

    auto p1 = alloc(64);
    Block *b1 = getHeader(p1);
    free(p1);

    auto p2 = alloc(16);
    Block *b2 = getHeader(p2);
    
    assert(b2 == b1);
    assert(b2->size == align(16));
    assert(b2->used == true);
    assert(b2->next != nullptr);
    assert(b2->next->used == false);

    std::cout << "✅ test_split_block_bestFit passed!\n";
    resetHeap();
}


void test_coalesce_blocks_bestFit(){
    init(SearchMode::BestFit);
    
    auto p1 = alloc(32);
    auto p2 = alloc(32);

    auto b1 = getHeader(p1);
    auto b2 = getHeader(p2);

    free(p2);
    free(p1);

    assert(b1->next == b2->next);
    assert(b1->size == align(32) + align(32) + headerSize());
    std::cout << "✅ test_coalesce_blocks_bestFit passed!\n";
    resetHeap();
}


void test_free_list_stategy(){
    init(SearchMode::FreeList);

    auto b1 = alloc(64);
    auto b2 = alloc(128);
    auto b3 = alloc(664);

    // Allocate 3 blocks

    assert(b1 != nullptr);
    assert(b2 != nullptr);
    assert(b3 != nullptr);

    // Free second block
    free(b2);
    assert(!free_list.empty());

    // Allocate new block of the same size as b2
    auto b4 = alloc(128);

    // It should reuse the block we just freed
    assert(b4 != nullptr);
    assert(b4 == b2);

    std::cout << "✅ test_free_list_stategy passed!\n";

    resetHeap();
}

void test_segregated_fit_strategy() {

    init(SearchMode::SegregatedFit);
    
    // Allocates blocks with size of buckets
    auto b8 = alloc(8);
    auto b16 = alloc(16);
    auto b32 = alloc(32);
    auto b64 = alloc(64);

    assert(b8 != nullptr);
    assert(b16 != nullptr);
    assert(b32 != nullptr);
    assert(b64 != nullptr);


    // Free the blocks
    free(b8);
    free(b16);
    free(b32);
    free(b64);

    /* 
    Store header info of each block in the appropriate bucket.
    This is to simulate the bucket population behavior
    */
    segregatedLists[0] = getHeader(b8);
    segregatedLists[1] = getHeader(b16);
    segregatedLists[2] = getHeader(b32);
    segregatedLists[3] = getHeader(b64);

    // Reallocate the freed blocks
    auto new_b8 = alloc(8);
    auto new_b16 = alloc(16);
    auto new_b32 = alloc(32);
    auto new_b64 = alloc(64);


    // Verify if the same blocks were used
    assert(new_b8 == b8);
    assert(new_b16 == b16);
    assert(new_b32 == b32);
    assert(new_b64 == b64);
    std::cout << "✅ test_segregated_fit_strategy passed!\n";
    
    resetHeap();
}


void test_segregated_fit_strategy_with_non_exact_sizes(){

    init(SearchMode::SegregatedFit);
    
    // Allocates blocks with size that don't match the bucket sizes
    auto b24 = alloc(24); // Goes in 32-byte bucket
    auto b40 = alloc(40); // Goes in 64-byte bucket
    auto b100 = alloc(100); // Goes in 128-byte bucket

    assert(b24 != nullptr);
    assert(b40 != nullptr);
    assert(b100 != nullptr);

    // Save the header of each block
    auto h24 = getHeader(b24);
    auto h40 = getHeader(b40);
    auto h100 = getHeader(b100);
    

    // Free the blocks
    free(b24);
    free(b40);
    free(b100);


    // Assign the block to the appropriate segregated lists manually
    segregatedLists[getBucket(h24->size)] = h24;
    segregatedLists[getBucket(h40->size)] = h40;
    segregatedLists[getBucket(h100->size)] = h100;

    
    // Allocate slightly smaller size to test reuse behavior from the buckets
    auto reused_b24 = alloc(20); // Should reuse from 32-byte bucket
    auto reused_b40 = alloc(36); // Should reuse from 64-byte bucket
    auto reused_b100 = alloc(100); // Should reuse from 28-byte bucket

    // Verify if the same blocks were reused
    assert(reused_b24 == b24);
    assert(reused_b24 == b24);
    assert(reused_b24 == b24);

    std::cout << "✅ test_segregated_fit_strategy_with_non_exact_sizes passed!\n";

    resetHeap();
}




int main(int argc, char const *argv[]) {
    test_basic_allocation_firstFit();
    test_nextFit_allocation_one();
    test_nextFit_allocation_two();
    test_bestFit_allocation();
    test_split_block_bestFit();
    test_coalesce_blocks_bestFit();
    test_free_list_stategy();
    test_segregated_fit_strategy();
    test_segregated_fit_strategy_with_non_exact_sizes();
    std::cout << "\nAll tests passed!\n";
    return 0;
}