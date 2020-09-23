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
//  Filename: SimpleMemAllocator.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Interfaces/pub/SystemControl.hpp"
#include "../pub/SimpleMemAllocator.hpp"

SimpleMemAllocator::SimpleMemAllocator(uint8_t *baseMemoryPtr, uint32_t maxSize) :
    baseMemoryPtr(baseMemoryPtr), maxSize(maxSize), size(0)
{
}

/**
 * Allocates a chunk of memory of the requested size (incremented to a multiple of 32 bytes)
 *
 * @param requestedSize the number of bytes to reserve
 * @return the pointer to the reserved memory block
 * @throw PeripheralException when the remaining space is less than the requested size
 */
uint8_t* SimpleMemAllocator::alloc(uint32_t requestedSize)
{
  requestedSize = (requestedSize + 31) & ~31;

  if ((requestedSize + size) > maxSize)
  {
    throw System::PeripheralException("Requested size exceeds maximum allocatable memory: " + requestedSize);
  }

  uint8_t *memPtr = &baseMemoryPtr[size];
  size += requestedSize;

  return memPtr;
}
