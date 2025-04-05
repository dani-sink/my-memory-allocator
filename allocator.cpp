#include <cstddef>
#include <cstdint>
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <cmath>
#include <list>

using word_t = intptr_t;

struct Block {
    size_t size;
    bool used;
    Block *next;
    // Block *prev;
    word_t data[1];
};

enum class SearchMode {
    FirstFit,
    NextFit,
    BestFit,
    FreeList,
    SegregatedFit,
};

static Block *heapStart = nullptr;

static auto top = heapStart;

static Block *searchStart = heapStart;

static auto searchMode = SearchMode::FirstFit;

static std::list<Block *> free_list;

Block *segregatedLists[] = {
    nullptr,  // 8 bytes
    nullptr,  // 16 bytes
    nullptr,  // 32 bytes
    nullptr,  // 64 bytes
    nullptr,  // 128 bytes
    nullptr,  // more than 128 bytes;
};

inline size_t align(size_t n){
    return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

inline size_t headerSize() {
    return sizeof(Block) - sizeof(std::declval<Block>().data);
}

inline size_t allocSize(size_t size) {
    return size + sizeof(Block) - sizeof(std::declval<Block>().data);
}

inline int getBucket(size_t size) {
    if (size <= 8) {
        return 0;
    } else if (size <= 16){
        return 1;
    } else if (size <= 32){
        return 2;
    } else if (size <= 64){
        return 3;
    } else if (size <= 128){
        return 4;
    } else {
        return 5;
    }    
}

void resetHeap(){
    if (heapStart == nullptr){
        return;
    }

    brk(heapStart);

    heapStart = nullptr;
    top = nullptr;
    searchStart = nullptr;

    free_list.clear();

    int length_seg_list = sizeof(segregatedLists) / sizeof(segregatedLists[0]);

    for (int i = 0; i < length_seg_list; i++){
        segregatedLists[i] = nullptr;
    }
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


Block *freeList(size_t size) {
    for (const auto &block : free_list){
        if (block->size < size) {
            continue;
        }

        free_list.remove(block);
        return listAllocate(block, size);
    }
    return nullptr;
}


Block *segregatedFit(size_t size){
    auto bucket = getBucket(size);
    auto originalHeapStart = heapStart;
    heapStart = segregatedLists[bucket];

    auto block = bestFit(size);

    heapStart = originalHeapStart;
    return block;
}


Block *findBlock(size_t size) {
    if (searchMode == SearchMode::FirstFit) {
        return firstFit(size);
    }
    if (searchMode == SearchMode::NextFit) {
        return nextFit(size);
    }
    if (searchMode == SearchMode::BestFit) {
        return bestFit(size);
    }
    if (searchMode == SearchMode::FreeList) {
        return freeList(size);
    }
    if (searchMode == SearchMode::SegregatedFit) {
        return segregatedFit(size);
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


bool canCoalesce(Block *block) {
    return block->next && !block->next->used;
}


Block *coalesce(Block *block){
    block->size += allocSize(block->next->size);
    block->next = block->next->next;
    return block;
}

void free(word_t *data) {
    auto block = getHeader(data);
    if (canCoalesce(block)) {
        block = coalesce(block);
    }
    block->used = false;
    if (searchMode == SearchMode::FreeList) {
        free_list.push_back(block);
    }
}

