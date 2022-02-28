#pragma once
#include <malloc.h>
#include <vector>
#include "MathCommon.h"
#define L1_CACHELINE_SIZE 64

#define ALLOCA(TYPE, COUNT) (TYPE *)alloca((COUNT) * sizeof(TYPE))
#define ARENA_ALLOC(arena, Type) new ((arena).Alloc(sizeof(Type))) Type

namespace RayTrace 
{


	inline void* AllocAligned(size_t size) {
		return _aligned_malloc(size, L1_CACHELINE_SIZE);
	}

	template <typename T> T* AllocAligned(std::size_t count) {
		return (T*)AllocAligned(count * sizeof(T));
	}


	inline void FreeAligned(void* ptr) {
		if (!ptr) return;
		_aligned_free(ptr);
	}

    class

        alignas(64)

        MemoryArena {
    public:
        // MemoryArena Public Methods
        MemoryArena(size_t blockSize = 262144) : blockSize(blockSize) {}
        ~MemoryArena() {
            FreeAligned(currentBlock);
            for (auto& block : usedBlocks) FreeAligned(block.second);
            for (auto& block : availableBlocks) FreeAligned(block.second);
        }
        void* Alloc(size_t nBytes) {

            const int align = alignof(std::max_align_t);
            static_assert(IsPowerOf2(align), "Minimum alignment not a power of two");

            nBytes = (nBytes + align - 1) & ~(align - 1);
            if (currentBlockPos + nBytes > currentAllocSize) {
                // Add current block to _usedBlocks_ list
                if (currentBlock) {
                    usedBlocks.push_back(
                        std::make_pair(currentAllocSize, currentBlock));
                    currentBlock = nullptr;
                    currentAllocSize = 0;
                }

                // Get new block of memory for _MemoryArena_

                // Try to get memory block from _availableBlocks_
                for (auto iter = availableBlocks.begin();
                    iter != availableBlocks.end(); ++iter) {
                    if (iter->first >= nBytes) {
                        currentAllocSize = iter->first;
                        currentBlock = iter->second;
                        availableBlocks.erase(iter);
                        break;
                    }
                }
                if (!currentBlock) {
                    currentAllocSize = std::max(nBytes, blockSize);
                    currentBlock = AllocAligned<uint8_t>(currentAllocSize);
                }
                currentBlockPos = 0;
            }
            void* ret = currentBlock + currentBlockPos;
            currentBlockPos += nBytes;
            return ret;
        }
        template <typename T>
        T* Alloc(size_t n = 1, bool runConstructor = true) {
            T* ret = (T*)Alloc(n * sizeof(T));
            if (runConstructor)
                for (size_t i = 0; i < n; ++i) new (&ret[i]) T();
            return ret;
        }
        void Reset() {
            currentBlockPos = 0;
            availableBlocks.splice(availableBlocks.begin(), usedBlocks);
        }
        size_t TotalAllocated() const {
            size_t total = currentAllocSize;
            for (const auto& alloc : usedBlocks) total += alloc.first;
            for (const auto& alloc : availableBlocks) total += alloc.first;
            return total;
        }

    private:
        MemoryArena(const MemoryArena&) = delete;
        MemoryArena& operator=(const MemoryArena&) = delete;
        // MemoryArena Private Data
        const size_t blockSize;
        size_t currentBlockPos = 0, currentAllocSize = 0;
        uint8_t* currentBlock = nullptr;
        std::list<std::pair<size_t, uint8_t*>> usedBlocks, availableBlocks;
    };


    template <typename T, int logBlockSize = 2> class BlockedArray {
    public:
        // BlockedArray Public Methods
        BlockedArray(uint32_t nu, uint32_t nv, const T* d = NULL) {
            uRes = nu;
            vRes = nv;
            uBlocks = RoundUp(uRes) >> logBlockSize;
            uint32_t nAlloc = RoundUp(uRes) * RoundUp(vRes);
            data = AllocAligned<T>(nAlloc);
            for (uint32_t i = 0; i < nAlloc; ++i)
                new (&data[i]) T();
            if (d)
                for (uint32_t v = 0; v < vRes; ++v)
                    for (uint32_t u = 0; u < uRes; ++u)
                        (*this)(u, v) = d[v * uRes + u];
        }
        uint32_t BlockSize() const { return 1 << logBlockSize; }
        uint32_t RoundUp(uint32_t x) const {
            return (x + BlockSize() - 1) & ~(BlockSize() - 1);
        }
        uint32_t uSize() const { return uRes; }
        uint32_t vSize() const { return vRes; }
        ~BlockedArray() {
            for (uint32_t i = 0; i < uRes * vRes; ++i)
                data[i].~T();
            FreeAligned(data);
        }
        uint32_t Block(uint32_t a) const { return a >> logBlockSize; }
        uint32_t Offset(uint32_t a) const { return (a & (BlockSize() - 1)); }
        T& operator()(uint32_t u, uint32_t v) {
            uint32_t bu = Block(u), bv = Block(v);
            uint32_t ou = Offset(u), ov = Offset(v);
            uint32_t offset = BlockSize() * BlockSize() * (uBlocks * bv + bu);
            offset += BlockSize() * ov + ou;
            return data[offset];
        }
        const T& operator()(uint32_t u, uint32_t v) const {
            uint32_t bu = Block(u), bv = Block(v);
            uint32_t ou = Offset(u), ov = Offset(v);
            uint32_t offset = BlockSize() * BlockSize() * (uBlocks * bv + bu);
            offset += BlockSize() * ov + ou;
            return data[offset];
        }
        void GetLinearArray(T* a) const {
            for (uint32_t v = 0; v < vRes; ++v)
                for (uint32_t u = 0; u < uRes; ++u)
                    *a++ = (*this)(u, v);
        }
    private:
        // BlockedArray Private Data
        T* data;
        uint32_t uRes, vRes, uBlocks;
    };
}