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
//  Description: Operating system abstraction layer - FreeRtos port
//  Filename: FreeRtosOal.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "../pub/Oal.hpp"
#include <cmsis_os2.h>

namespace System
{

/**
 * This class wraps the FreeRtos OS operations
 * of the OS layer with the rest of the code.
 */
class FreeRtosOal: public Oal
{
public:
  void* createMessageQueue(uint32_t msg_count, uint32_t msg_size) override;
  void sendMessageToQueue(void *queue, const void *msg_ptr, uint32_t timeout) override;
  uint32_t popMessageFromQueue(void *queue, void *msg_ptr, uint32_t timeout) override;
  OalTask* startTask(char *name, uint32_t stackSize, OalTaskPriority priority, void (*func)(void*), void *argument) override;
  void delay(uint32_t ticks) override;

  void sendTaskNotification(volatile void *rxTaskHandle) override;
  void waitForTaskNotification(uint32_t timeout) override;
};

class FreeRtosTask: public OalTask
{
private:
  osThreadId_t taskHandle;
  osThreadAttr_t taskAttr;

public:
  FreeRtosTask(char *name, uint32_t stackSize, osPriority_t priority, osThreadFunc_t func, void *argument);
};


}
