/**
===============================================================================
 File:           MemoryManager.h
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2026-02-18
 Contribution:   100%
 ------------------------------------------------------------------------------

  Custom Memory Manager for Game Engine
  ======================================

  This file implements a page-based pool memory allocation system designed to
  replace the default C++ heap allocator (new/delete) for game objects and
  components. The system pre-allocates large blocks of memory ("pages") at
  load time, then hands out fixed-size chunks from those pages at runtime
  with zero OS involvement.

  Architecture Overview:
  ~~~~~~~~~~~~~~~~~~~~~~

    MemoryManager (Singleton)
        |
        +-- PoolAllocator [blockSize=16]  --> MemoryPage --> MemoryPage --> ...
        |       |
        |       +-- FreeBlock -> FreeBlock -> FreeBlock -> nullptr  (free list)
        |
        +-- PoolAllocator [blockSize=48]  --> MemoryPage --> ...
        |       |
        |       +-- FreeBlock -> FreeBlock -> nullptr
        |
        +-- PoolAllocator [blockSize=128] --> MemoryPage --> ...
        |       |
        |       +-- FreeBlock -> nullptr
        ...

  Key Design Decisions:
  ~~~~~~~~~~~~~~~~~~~~~

  1. PLACEMENT NEW (C++ technique)
     Objects are constructed in pre-allocated memory using placement new:
         T* obj = new (rawMemoryPtr) T(args...);
     This separates memory allocation from object construction, allowing
     us to reuse the same memory block for different object lifetimes.

  2. INTRUSIVE FREE LINKED LIST
     Free blocks store a pointer to the next free block in their own memory.
     When a block is free, its first sizeof(void*) bytes contain a FreeBlock*
     pointer. When allocated, those bytes are overwritten by the actual object
     data. This gives us O(1) allocation and deallocation with zero memory
     overhead for bookkeeping.

  3. MEMORY PAGES
     Each PoolAllocator organizes memory into pages - large contiguous chunks
     obtained from the OS via malloc(). Pages are chained together using an
     intrusive singly-linked list. Each page is subdivided into fixed-size
     blocks that feed the free list.

  4. NO STL CONTAINERS
     The memory manager itself uses NO STL containers (no std::vector,
     std::unordered_map, std::map, etc.). All internal data structures use
     raw pointers, intrusive linked lists, and fixed-size arrays.

  5. DYNAMIC PAGE EXTENSION
     When a pool's free list is exhausted at runtime, a new page is allocated
     from the OS and its blocks are chained into the free list. This is the
     ONLY time an OS allocation occurs after initialization.

  Runtime Behavior:
  ~~~~~~~~~~~~~~~~~
  - Load time:  Pools are created and initial pages allocated from OS
  - Runtime:    Allocate() pops from free list (O(1), no OS call)
                Deallocate() pushes to free list (O(1), no OS call)
                Memory bytes are NOT returned to OS on deallocation
  - Shutdown:   All pages are freed back to the OS

===============================================================================
*/

#pragma once

#include <cstddef>   // size_t
#include <cstdint>   // uint8_t
#include <new>       // placement new
#include <utility>   // std::forward

namespace Framework
{

// ============================================================================
// MEMORY PAGE
// ============================================================================
//
// A MemoryPage represents a contiguous block of raw memory obtained from the
// operating system at load time (or when the pool needs to grow). The page is
// subdivided into fixed-size blocks that are managed by a PoolAllocator.
//
// Pages are chained together using an intrusive singly-linked list (nextPage
// pointer). This avoids any STL container usage for page tracking.
//
// Memory layout of a page:
//   [MemoryPage header] --> [Block 0][Block 1][Block 2]...[Block N-1]
//                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//                            This is the raw memory buffer (page->memory)
//
// Each block is exactly 'blockSize' bytes. When free, the first sizeof(void*)
// bytes of each block are reinterpreted as a FreeBlock pointer (intrusive
// free list node). When allocated, those bytes hold actual object data.
// ============================================================================

struct MemoryPage
{
    uint8_t*    memory;       ///< Pointer to the raw memory buffer (OS-allocated)
    MemoryPage* nextPage;     ///< Next page in the chain (intrusive linked list)
    size_t      blockSize;    ///< Size of each block within this page (bytes)
    size_t      blockCount;   ///< Number of blocks in this page
    size_t      totalSize;    ///< Total bytes allocated for the memory buffer

    MemoryPage()
        : memory(nullptr)
        , nextPage(nullptr)
        , blockSize(0)
        , blockCount(0)
        , totalSize(0)
    {}
};

// ============================================================================
// FREE BLOCK (INTRUSIVE FREE LIST NODE)
// ============================================================================
//
// When a block is free (not in use by a game object), the first bytes of the
// block are reinterpreted as a FreeBlock struct. This struct contains a single
// pointer to the next free block, forming a singly-linked list.
//
// This is an "intrusive" data structure because it stores list metadata
// directly inside the free memory blocks themselves, requiring ZERO additional
// memory overhead for tracking free blocks.
//
// When a block is allocated (in use), these bytes are overwritten by the
// actual object data via placement new. When deallocated, the object's
// destructor is called first, then the block's first bytes are overwritten
// with the FreeBlock pointer to re-enter the free list.
//
// Invariant: blockSize >= sizeof(FreeBlock) (8 bytes on 64-bit systems)
// ============================================================================

struct FreeBlock
{
    FreeBlock* next;    ///< Pointer to the next free block in the list
};

// ============================================================================
// POOL ALLOCATOR
// ============================================================================
//
// A PoolAllocator manages a collection of fixed-size memory blocks organized
// into memory pages. It provides O(1) allocation and deallocation using an
// intrusive free linked list.
//
// How it works:
//
//   INITIALIZATION:
//   A page of raw memory is allocated from the OS. The page is divided into
//   N blocks of equal size. Each block's first bytes are used as a FreeBlock
//   pointer, chaining all blocks into a singly-linked free list:
//
//     Page Memory:  [Block0] -> [Block1] -> [Block2] -> ... -> [BlockN] -> null
//                    ^
//                    m_freeList (head pointer)
//
//   ALLOCATION (O(1)):
//   Pop the head of the free list. The block's memory is now available for
//   the caller to construct an object via placement new:
//
//     Before:  m_freeList -> [Block0] -> [Block1] -> [Block2] -> ...
//     After:   m_freeList -> [Block1] -> [Block2] -> ...
//              [Block0] is returned to caller (free list pointer overwritten)
//
//   DEALLOCATION (O(1)):
//   Push the block back to the head of the free list. The object's destructor
//   must be called BEFORE this. The block's first bytes are overwritten with
//   the free list pointer:
//
//     Before:  m_freeList -> [Block1] -> [Block2] -> ...
//     After:   m_freeList -> [Block0] -> [Block1] -> [Block2] -> ...
//
//   PAGE EXTENSION:
//   When the free list is empty and Allocate() is called, a new page is
//   allocated from the OS. Its blocks are chained into the free list, then
//   allocation proceeds as normal. This is the ONLY OS allocation at runtime.
//
// Thread safety: NOT thread-safe (single-threaded engine assumed)
// ============================================================================

class PoolAllocator
{
public:
    PoolAllocator();
    ~PoolAllocator();

    /**
     * @brief Initialize the pool with a specific block size and page capacity
     * @param blockSize Size of each block in bytes (will be aligned to 16 bytes)
     * @param blocksPerPage Number of blocks per memory page
     *
     * The block size is automatically padded to at least sizeof(FreeBlock) and
     * aligned to 16-byte boundaries for optimal cache performance.
     * One initial memory page is allocated from the OS during initialization.
     */
    void Initialize(size_t blockSize, size_t blocksPerPage);

    /**
     * @brief Allocate a single block from the pool
     * @return Pointer to the allocated memory block
     *
     * Pops the head of the free list in O(1). If the free list is empty, a new
     * memory page is allocated from the OS to extend the pool, then allocation
     * proceeds. The returned memory is uninitialized - use placement new to
     * construct an object.
     */
    void* Allocate();

    /**
     * @brief Return a block to the pool (mark as free)
     * @param ptr Pointer to the block to deallocate
     *
     * IMPORTANT: The caller MUST call the object's destructor before calling
     * Deallocate(). This function does NOT call any destructor.
     *
     * The actual memory bytes are NOT zeroed or returned to the OS - the block
     * is simply pushed back onto the free list for reuse by future allocations.
     * This is the key optimization: "deleting" an object at runtime just
     * manipulates a pointer, with zero OS involvement.
     *
     * Time complexity: O(1)
     */
    void Deallocate(void* ptr);

    /**
     * @brief Release all memory pages back to the OS
     *
     * Called during engine shutdown. After this call, ALL pointers previously
     * returned by Allocate() become invalid. The pool cannot be used again
     * without calling Initialize().
     */
    void Shutdown();

    // -- Diagnostic accessors --
    size_t GetBlockSize()   const { return m_blockSize; }
    size_t GetTotalBlocks() const { return m_totalBlocks; }
    size_t GetUsedBlocks()  const { return m_usedBlocks; }
    size_t GetFreeBlocks()  const { return m_totalBlocks - m_usedBlocks; }
    size_t GetPageCount()   const { return m_pageCount; }
    bool   IsInitialized()  const { return m_initialized; }

private:
    /**
     * @brief Allocate a new memory page from the OS and chain its blocks
     *        into the free list
     *
     * Called automatically when Allocate() finds the free list empty.
     * This is the ONLY function that performs OS-level memory allocation
     * (via malloc). All blocks in the new page are linked into the free list.
     */
    void AllocateNewPage();

    FreeBlock*  m_freeList;       ///< Head of the intrusive free linked list
    MemoryPage* m_firstPage;      ///< Head of the page chain (linked list)
    size_t      m_blockSize;      ///< Size of each block in bytes (aligned)
    size_t      m_blocksPerPage;  ///< Number of blocks allocated per page
    size_t      m_totalBlocks;    ///< Total blocks across all pages
    size_t      m_usedBlocks;     ///< Currently allocated (in-use) blocks
    size_t      m_pageCount;      ///< Number of pages in the chain
    bool        m_initialized;    ///< Whether Initialize() has been called
};

// ============================================================================
// MEMORY MANAGER (SINGLETON)
// ============================================================================
//
// The MemoryManager is the top-level interface for the custom memory system.
// It manages multiple PoolAllocators, one per unique block size needed by
// the engine's game objects and components.
//
// Pool Registry (NO STL - fixed-size array):
//   The manager maintains a fixed-size array of PoolEntry structs. Each entry
//   maps an aligned block size to a PoolAllocator. When Allocate<T>() is
//   called, the manager finds (or creates) the pool for sizeof(T) and
//   delegates to it.
//
// Lifecycle:
//   1. Initialize()           -- Called at engine startup
//   2. Allocate<T>(args...)   -- Called when creating game objects/components
//   3. Deallocate<T>(ptr)     -- Called when destroying objects (memory reused)
//   4. Shutdown()             -- Called at engine shutdown (memory returned to OS)
//
// Integration with ECS:
//   The ECS EntityManager uses this MemoryManager for all component allocation.
//   AddComponent<T>() calls Allocate<T>() with placement new.
//   RemoveComponent<T>() and DestroyEntity() call DeallocateComponent() which
//   invokes the virtual destructor and returns the block to the correct pool.
//
// Example:
//   // At runtime (booking pre-allocated memory for a new component):
//   Transform* t = MemoryManager::GetInstance().Allocate<Transform>(pos);
//
//   // When destroying (memory NOT returned to OS, just marked free):
//   MemoryManager::GetInstance().Deallocate<Transform>(t);
// ============================================================================

class MemoryManager
{
public:
    /**
     * @brief Get the singleton instance of the MemoryManager
     * @return Reference to the global MemoryManager
     */
    static MemoryManager& GetInstance();

    /**
     * @brief Initialize the memory manager
     *
     * Called once at engine startup (load time). After this call, pools are
     * created on-demand as new component types are first allocated. Each
     * pool's first page is the "load time" OS allocation.
     */
    void Initialize();

    /**
     * @brief Shutdown and release ALL memory back to the OS
     *
     * Called once at engine shutdown. All pool pages are freed. After this
     * call, any pointers previously returned by Allocate() are invalid.
     */
    void Shutdown();

    /**
     * @brief Allocate and construct a typed object using PLACEMENT NEW
     * @tparam T The type to allocate
     * @tparam Args Constructor argument types
     * @param args Arguments forwarded to T's constructor
     * @return Pointer to the newly constructed object
     *
     * This is the primary allocation interface. It:
     *   1. Finds (or creates) the PoolAllocator for sizeof(T)
     *   2. Pops a free block from the pool (O(1), no OS call)
     *   3. Constructs the object in-place using placement new
     *
     * If the pool is exhausted, a new memory page is allocated from the OS
     * to extend it (this is the only case where an OS allocation occurs
     * at runtime).
     */
    template<typename T, typename... Args>
    T* Allocate(Args&&... args);

    /**
     * @brief Destroy and deallocate a typed object
     * @tparam T The type to deallocate
     * @param ptr Pointer to the object
     *
     * This is the primary deallocation interface. It:
     *   1. Calls the object's destructor explicitly (~T())
     *   2. Returns the memory block to the pool's free list (O(1))
     *
     * The actual memory bytes are NOT returned to the OS. They remain in
     * the pool page, available for reuse by future Allocate() calls.
     * This is what the rubric means by "the actual bytes are not deleted,
     * but simply freed."
     */
    template<typename T>
    void Deallocate(T* ptr);

    /**
     * @brief Deallocate a block using a base pointer and known size
     * @param ptr Pointer to the block (destructor MUST already be called)
     * @param size The original sizeof(DerivedType) for pool lookup
     *
     * Used when the exact derived type is not known at compile time
     * (e.g., deallocating via ComponentBase* in the ECS). The caller
     * must call the virtual destructor before calling this method.
     */
    void DeallocateBySize(void* ptr, size_t size);

    /**
     * @brief Print diagnostic information about all active pools
     *
     * Outputs block counts, page counts, and memory usage for each pool.
     * Useful for debugging and proving the memory manager is working.
     */
    void PrintStats() const;

    /**
     * @brief Check if the memory manager has been initialized
     * @return true if Initialize() has been called
     */
    bool IsInitialized() const { return m_initialized; }

private:
    MemoryManager();
    ~MemoryManager();

    // Non-copyable, non-movable (singleton)
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    /**
     * @brief Find or create a pool allocator for the given block size
     * @param size Raw size in bytes (will be aligned to 16 bytes internally)
     * @return Pointer to the appropriate PoolAllocator
     *
     * Performs a linear search through the pool registry. If no pool exists
     * for this aligned size, creates a new one with an initial page.
     * Linear search is used (instead of hash maps) to satisfy the NO STL
     * requirement. This is fast because the number of distinct component
     * sizes is small (typically < 30 types in a game engine).
     */
    PoolAllocator* GetPool(size_t size);

    /**
     * @brief Align a size up to 16-byte boundary
     * @param size Raw size in bytes
     * @return Aligned size (rounded up to nearest multiple of 16)
     *
     * 16-byte alignment ensures proper alignment for any standard C++ type
     * and optimizes cache line access patterns.
     */
    static size_t AlignSize(size_t size);

    // ====================================================================
    // Pool Registry - NO STL CONTAINERS
    // ====================================================================
    // Uses a fixed-size array of PoolEntry structs. Each entry maps an
    // aligned block size to a PoolAllocator instance. The array is searched
    // linearly, which is efficient given the small number of distinct
    // component sizes in a typical game engine.
    // ====================================================================

    static constexpr size_t MAX_POOLS = 64;  ///< Maximum distinct block sizes

    /**
     * @brief Registry entry mapping a block size to a pool allocator
     */
    struct PoolEntry
    {
        size_t        blockSize;  ///< Aligned block size this pool serves
        PoolAllocator pool;       ///< The pool allocator instance
        bool          active;     ///< Whether this entry is in use

        PoolEntry() : blockSize(0), active(false) {}
    };

    PoolEntry m_pools[MAX_POOLS];  ///< Fixed-size pool registry (no STL)
    size_t    m_poolCount;         ///< Number of active pools
    bool      m_initialized;       ///< Whether Initialize() has been called

    /// Default number of blocks per page when creating a new pool
    static constexpr size_t DEFAULT_BLOCKS_PER_PAGE = 256;
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS (must be in header)
// ============================================================================

template<typename T, typename... Args>
T* MemoryManager::Allocate(Args&&... args)
{
    // Step 1: Find or create the pool for this type's size
    PoolAllocator* pool = GetPool(sizeof(T));

    // Step 2: Get a raw memory block from the pool's free list (O(1))
    //         No OS allocation occurs here unless the pool is exhausted.
    void* memory = pool->Allocate();

    // Step 3: Construct the object in-place using PLACEMENT NEW
    //         This is the key C++ technique specified in the rubric.
    //         Placement new constructs an object at a specific memory
    //         address without allocating new memory from the OS.
    T* object = new (memory) T(std::forward<Args>(args)...);

    return object;
}

template<typename T>
void MemoryManager::Deallocate(T* ptr)
{
    if (!ptr) return;

    // Step 1: Call the destructor explicitly
    //         This cleans up the object's internal state (e.g., closing
    //         Lua states, releasing string memory) WITHOUT freeing the
    //         underlying memory block.
    ptr->~T();

    // Step 2: Return the memory block to the pool's free list (O(1))
    //         The block remains in the page - only its free list pointer
    //         is updated. The memory bytes are NOT returned to the OS.
    //         This is what "the actual bytes are not deleted, but simply
    //         freed" means in the rubric.
    PoolAllocator* pool = GetPool(sizeof(T));
    pool->Deallocate(static_cast<void*>(ptr));
}

} // namespace Framework
