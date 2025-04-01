#include <cstddef>
#include <cstdint>
#include <iostream>
#include <unistd.h>
#include <assert.h>

using word_t = intptr_t;

struct Block {
    size_t size;
    bool used;
    Block *next;
    word_t data[1];
};

enum class SearchMode {
    FirstFit,
    NextFit,
    BestFit,
};

static Block *heapStart = nullptr;

static auto top = heapStart;

static Block *searchStart = heapStart;

static auto searchMode = SearchMode::FirstFit;

inline size_t align(size_t n){
    return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

inline size_t allocSize(size_t size) {
    return size + sizeof(Block) - sizeof(std::declval<Block>().data);
}

void resetHeap(){
    if (heapStart == nullptr){
        return;
    }

    brk(heapStart);

    heapStart = nullptr;
    top = nullptr;
    searchStart = nullptr;
}

void init(SearchMode mode) {
    searchMode = mode;
    resetHeap();
}


Block *firstFit(size_t size) {
    auto block = heapStart;

    while (block != nullptr) {
        if (block->used || block->size < size) {
            block = block->next;
            continue;
        }

        return block;
    }

    return nullptr;
}

Block *nextFit(size_t size) {

    // start from previous successful position (if there is one)
    auto block = searchStart;
    while (block != nullptr) {
        if (block->used || block->size < size){
            block = block->next;
            continue;
        }
        // found the block
        // This is the current succesful position set searchStart to this block
        searchStart = block;
        block->used = true;
        return block;
    }

    // start again from the beginning
    block = heapStart;

    while (block != searchStart){
        if (block->used || block->size < size){
            block = block->next;
            continue;
        }

        // found the block
        // This is the current succesful position set searchStart to this block
        searchStart = block;
        block->used = true;
        return block;
    }

    // if we reach this point, we have not found a suitable block,
    // even after a circular traversal fom searchStart
    return nullptr;
}

Block *bestFit(size_t size){
    auto block = heapStart;
    auto currentBestBlock = heapStart;
    auto found = false;
    while (block != nullptr) {
        if (block->used || block->size < size) {
            block = block->next;
            continue;
        }

        found = true;
        auto currentBlockSizeDiff = block->size - size;
        if (currentBlockSizeDiff == 0) {
            currentBestBlock = block;
            currentBestBlock->used = true;
            return currentBestBlock;
        }
        else if (currentBestBlock == heapStart){
            currentBestBlock = block;
            block = block->next;
        } else {
            auto currentBestBlockSizeDiff = currentBestBlock->size - size;
            if (currentBlockSizeDiff < currentBestBlockSizeDiff) {
                currentBestBlock = block;
            }
        }
    }

    if (found == true) {
        currentBestBlock->used = true;
        return currentBestBlock;
    } else {
        return nullptr;
    }

}

Block *findBlock(size_t size) {
    if (searchMode == SearchMode::FirstFit) {
        return firstFit(size);
    }
    else if (searchMode == SearchMode::NextFit) {
        return nextFit(size);
    }
    else if (searchMode == SearchMode::BestFit) {
        return bestFit(size);
    }
    else {
        return nullptr;
    }
}

Block *requestFromOS(size_t size){
    auto block = (Block *)sbrk(0);

    if (sbrk(allocSize(size)) == (void *)-1) {
        return nullptr;
    }

    return block;
}

word_t *alloc(size_t size) {
    size = align(size);

    if (auto block = findBlock(size)) {
        return block->data;
    }
    
    auto block = requestFromOS(size);

    block->size = size;
    block->used = true;

    if (heapStart == nullptr) {
        heapStart = block;
    }

    if (top != nullptr) {
        top->next = block;
    }

    top = block;

    return block->data;
}


Block *getHeader(word_t *data) {
    return (Block *)((char *)data + sizeof(std::declval<Block>().data) - sizeof(Block));
}

void free(word_t *data) {
    auto block = getHeader(data);
    block->used = false;
}




int main(int argc, char const *argv[]) {
    // std::cout << allocSize(12) << std::endl;

    // auto p1 = alloc(3);
    // auto p1b = getHeader(p1);
    // assert(p1b->size == sizeof(word_t));

    // auto p2 = alloc(8);
    // auto p2b = getHeader(p2);
    // assert(p2b->size == 8);

    // free(p2);
    // assert(p2b->used == false);

    // auto p3 = alloc(8);
    // auto p3b = getHeader(p3);
    // assert(p3b->size == 8);
    // assert(p3b == p2b);


    /* NextFit testcases*/

    /* Test Case 1*/

    // init(SearchMode::NextFit);
    // auto o1 = alloc(8);
    // auto o2 = alloc(8);
    // auto o3 = alloc(8);
    
    // // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
    // auto o4 = alloc(16);
    // auto o5 = alloc(16);
    
    // // [[8, 1], [8, 1], [8, 1], [16, 0], [16, 0]]
    // free(o4);
    // free(o5);
    
    // // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
    // auto o6 = alloc(16);
    
    // // Start position from o6:
    // assert(searchStart == getHeader(o6));
    // // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
    // //                           ^ start here

    // alloc(16);

    // init(SearchMode::NextFit);

    // auto a1 = alloc(8);
    // auto a2 = alloc(8);
    // auto a3 = alloc(8);

    // auto a4 = alloc(16);
    // auto a5 = alloc(16);


    // free(a4);
    // free(a5);

    // // Allocate a16 again, should go into a4's slot
    // auto a6 = alloc(16);
    // assert(getHeader(a6) == getHeader(a4));
    // assert(searchStart == getHeader(a6));
    // // Test wrap-around behavior
    // auto a7 = alloc(16); // This should go into a5
    // assert(getHeader(a7) == getHeader(a5));
    // assert(searchStart == getHeader(a7));

    // // Now the heap has no free 16-byte blocks
    // // Allocate a larger block to force sbrk (new memory)
    // auto a8 = alloc(32);
    // assert(getHeader(a8)->size == align(32));
    // assert(getHeader(a8)->used == true);

    // // // Check that the searchStart is not pointing to a reused free block
    // auto a9 = alloc(1000);
    // assert(getHeader(a9)->used == true);

    // std::cout << getHeader(a1) << std::endl;
    // std::cout << getHeader(a2) << std::endl;
    // std::cout << getHeader(a3) << std::endl;
    // std::cout << getHeader(a4) << std::endl;
    // std::cout << getHeader(a5) << std::endl;
    // std::cout << getHeader(a6) << std::endl;
    // std::cout << getHeader(a7) << std::endl;
    // std::cout << getHeader(a8) << std::endl;
    // std::cout << getHeader(a9) << std::endl;

    init(SearchMode::BestFit);
 
    // --------------------------------------
    // Test case 6: Best-fit search
    //
    
    // [[8, 1], [64, 1], [8, 1], [16, 1]]
    alloc(8);
    auto z1 = alloc(64);
    alloc(8);
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
    
    // // Reuse 64, splitting it to 16, and 24 (considering header)
    z3 = alloc(16);
    assert(getHeader(z3) == getHeader(z1));
    
    // [[8, 1], [16, 1], [24, 0], [8, 1], [16, 1]]

    puts("\nAll assertions passed!\n");
    return 0;
}