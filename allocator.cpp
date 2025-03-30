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

static Block *heapStart = nullptr;

static auto top = heapStart;

inline size_t align(size_t n){
    return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

inline size_t allocSize(size_t size) {
    return size + sizeof(Block) - sizeof(std::declval<Block>().data);
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
    auto p1 = alloc(3);
    auto p1b = getHeader(p1);
    assert(p1b->size == sizeof(word_t));

    auto p2 = alloc(8);
    auto p2b = getHeader(p2);
    assert(p2b->size == 8);

    free(p2);
    assert(p2b->used == false);
    puts("\nAll assertions passed!\n");
    return 0;
}