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
        return block;
    }

    // if we reach this point, we have not found a suitable block,
    // even after a circular traversal fom searchStart
    return nullptr;
}

Block *findBlock(size_t size) {
    if (searchMode == SearchMode::FirstFit) {
        return firstFit(size);
    }
    else if (searchMode == SearchMode::NextFit) {
        return nextFit(size);
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

    puts("\nAll assertions passed!\n");
    return 0;
}