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
//  Description: Operating system abstraction layer
//  Filename: Oal.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include <stdint.h>

namespace System
{

enum OalTaskPriority
{
  PRIO_LOW,
  PRIO_MEDIUM,
  PRIO_HIGH,
  PRIO_EXTREME

};

/**
 * Abstract interface to wrap the OS tasks
 */
class OalTask
{
public:
};

/**
 * This interface contains the basic OS operations and allows decoupling
 * of the OS layer with the rest of the code.
 */
class Oal
{
public:
  virtual void* createMessageQueue(uint32_t msg_count, uint32_t msg_size) = 0;
  virtual uint32_t popMessageFromQueue(void *queue, void *msg_ptr, uint32_t timeout) = 0;
  virtual uint32_t popMessageFromQueueFromISR(void *queue, void *msg_ptr, uint32_t timeout) = 0;
  virtual void sendMessageToQueue(void *queue, const void *msg_ptr, uint32_t timeout) = 0;
  virtual OalTask* startTask(char *name, uint32_t stackSize, OalTaskPriority priority, void (*func)(void*), void *argument) = 0;
  virtual void delay(uint32_t ticks) = 0;

  virtual void sendTaskNotification(volatile void *rxTaskHandle) = 0;
  virtual void waitForTaskNotification(uint32_t timeout) = 0;

  virtual ~Oal()
  {
  }
};

/**
 * Helper factory class to return the Oal instance
 */
class OalFactory
{
public:
  Oal* getOal();
};


}
