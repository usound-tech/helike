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
//  Filename: FreeRtosOal.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/Oal.hpp"
#include "FreeRtosOal.hpp"
#include "FreeRTOS.h"
#include "queue.h"

namespace System
{

void FreeRtosOal::sendMessageToQueue(void *queue, const void *msg_ptr, uint32_t timeout)
{
  osMessageQueuePut(queue, msg_ptr, 0, timeout);
}

uint32_t FreeRtosOal::popMessageFromQueue(void *queue, void *msg_ptr, uint32_t timeout)
{
  return xQueueReceive((QueueHandle_t) queue, msg_ptr, timeout);
}

uint32_t FreeRtosOal::popMessageFromQueueFromISR(void *queue, void *msg_ptr, uint32_t timeout)
{
  BaseType_t xTaskWokenByReceive = pdFALSE;

  xQueueReceiveFromISR((QueueHandle_t) queue, msg_ptr, &xTaskWokenByReceive);
  if (xTaskWokenByReceive != pdFALSE)
  {
    taskYIELD();
  }

  return pdPASS;
}

/**
 * Creates a new FreeRtos task and starts it immediately.
 * @param name
 * @param stackSize
 * @param priority
 * @param func
 * @param argument
 * @return
 */
OalTask* FreeRtosOal::startTask(char *name, uint32_t stackSize, OalTaskPriority priority, void (*func)(void*), void *argument)
{
  osPriority_t freeRtosPriority;

  switch (priority)
  {
    case OalTaskPriority::PRIO_EXTREME:
      freeRtosPriority = osPriorityRealtime;
      break;

    case OalTaskPriority::PRIO_HIGH:
      freeRtosPriority = osPriorityHigh;
      break;

    case OalTaskPriority::PRIO_MEDIUM:
      freeRtosPriority = osPriorityNormal;
      break;

    default:
      case OalTaskPriority::PRIO_LOW:
      freeRtosPriority = osPriorityLow;
      break;
  }

  return new FreeRtosTask(name, stackSize, freeRtosPriority, static_cast<osThreadFunc_t>(func), argument);
}

/**
 * Yields the task for at least "ticks" elapsed time
 * @param ticks
 */
void FreeRtosOal::delay(uint32_t ticks)
{
  osDelay(ticks);
}

/**
 * Creates a new message queue
 * @param msg_count
 * @param msg_size
 */
void* FreeRtosOal::createMessageQueue(uint32_t msg_count, uint32_t msg_size)
{
  return xQueueCreate(msg_count, msg_size);
}

/**
 * Waits for an event (boolean type)
 */
void FreeRtosOal::waitForTaskNotification(uint32_t timeout)
{
  ulTaskNotifyTake(pdTRUE, timeout);
}

/**
 * Sends a notification from ISR to a waiting task
 * @param rxTaskHandle
 */
void FreeRtosOal::sendTaskNotification(volatile void *rxTaskHandle)
{
  if (rxTaskHandle)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR((TaskHandle_t) rxTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/**
 * Creates a new FreeRtos task and wraps it inside a class
 * @param name
 * @param stackSize
 * @param priority
 * @param func
 * @param argument
 */
FreeRtosTask::FreeRtosTask(char *name, uint32_t stackSize, osPriority_t priority, osThreadFunc_t func, void *argument) :
    taskAttr( { 0 })
{
  taskAttr.name = name;
  taskAttr.stack_size = stackSize;
  taskAttr.priority = priority;

  taskHandle = osThreadNew(func, argument, &taskAttr);
}

}
