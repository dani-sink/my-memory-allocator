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

inline size_t headerSize() {
    return sizeof(Block) - sizeof(std::declval<Block>().data);
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

Block *split(Block *block, size_t size){
    auto originalSize = block->size;
    auto *oldNext = block->next;
    Block *leftBlock = block;

    Block *rightBlock = (Block*)((char*)leftBlock + allocSize(size));

    rightBlock->size = (originalSize + headerSize()) - allocSize(size) - headerSize();
    rightBlock->used = false;
    rightBlock->next = block->next;
    leftBlock->size = size;
    leftBlock->used = true;
    leftBlock->next = rightBlock;
    return leftBlock;
}

inline bool canSplit(Block *block, size_t size) {
    if (block->size < size) {
        return false;
    }

    return block->size - size >= headerSize() + align(8); 
}

Block *listAllocate(Block *block, size_t size){
    
    if (canSplit(block, size)) {
        block = split(block, size);
    }


    block->used = true;
    block->size = size;

    return block;
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
        return listAllocate(currentBestBlock, size);
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

