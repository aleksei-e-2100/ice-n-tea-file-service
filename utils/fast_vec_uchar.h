/**
 * @file no_init_allocator.h
 * @brief An allocator that does not initialize objects upon creation
 */

#pragma once

#include <vector>

/**
 * @struct NoInitAllocator
 * @brief Implements an allocator that does not initialize the objects it creates.
 *        Helps avoid unnecessary copying when creating an object.
 */
template <typename T, typename Allocator = std::allocator<T>>
struct NoInitAllocator : Allocator
{
    /**
     * @brief Create an object without initialization
     * @param p Pointer to the object being placed
     * @param args Constructor arguments
     */
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {}
};

/**
 * @brief A vector of bytes (unsigned char) with an allocator without initialization
 */
using FastVecUchar = std::vector<unsigned char, NoInitAllocator<unsigned char>>;
