//====================================================================
//
// COPYRIGHT 2020 All rights reserved.
//       USound
//
//====================================================================
//
//                          DISCLAIMER
// NO WARRANTIES
// USound expressly disclaims any warranty for the SOFTWARE
// PRODUCT. The SOFTWARE PRODUCT and any related documentation is
// provided "as is" without warranty of any kind, either expressed or
// implied, including, without limitation, the implied warranties or
// merchantability, fitness for a particular purpose, or noninfringe-
// ment. The entire risk arising out of the use or performance of the
// SOFTWARE PRODUCT remains with the user.
//
// NO LIABILITY FOR DAMAGES.
// Under no circumstances is USound liable for any damages
// whatsoever (including, without limitation, damages for loss of busi-
// ness profits, business interruption, loss of business information,
// or any other pecuniary loss) arising out of the use of or inability
// to use this product.
//
//====================================================================
//
//  Description: Simple memory allocator that reserves contiguous memory
//               chunks. It does not support memory deallocation.
//  Filename: SimpleMemAllocator.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================


#pragma once
#include <stdint.h>

/**
 * This class implements a very simple memory allocator.
 * It reserves contiguous blocks of memory and does not support freeing of the memory.
 * It's useful when initialising the system and reserving DMA memory
 */
class SimpleMemAllocator
{
private:
  uint8_t *baseMemoryPtr;
  uint32_t maxSize;
  uint32_t size;

public:
  SimpleMemAllocator(uint8_t *baseMemoryPtr, uint32_t requestedSize);

  uint8_t* alloc(uint32_t requestedSize);
};
