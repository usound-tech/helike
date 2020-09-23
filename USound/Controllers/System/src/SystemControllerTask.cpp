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
//  Description: This is the system controller thread.
//  Filename: SystemControllerTask.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/Service/pub/Services.hpp"
#include "../pub/SystemController.hpp"
#include "../pub/SystemStatus.hpp"
#include "main.h"
#include "cmsis_os2.h"

namespace System
{

void SystemStatus::normalTimeoutEventCb(void *arg)
{
  SystemStatus *systemStatus = static_cast<SystemStatus*>(arg);
  systemStatus->normalTimeoutEvent();
}

void SystemStatus::fastTimeoutEvenCb(void *arg)
{
  SystemStatus *systemStatus = static_cast<SystemStatus*>(arg);
  systemStatus->fastTimeoutEvent();
}

void SystemStatus::oneOffTimeoutEvenCb(void *arg)
{
  SystemStatus *systemStatus = static_cast<SystemStatus*>(arg);
  systemStatus->oneOffTimeoutEvent();
}


SystemStatus::SystemStatus()
{

}

void SystemStatus::init()
{
  timerNormal = osTimerNew(SystemStatus::normalTimeoutEventCb, osTimerPeriodic, this, nullptr);
  timerFast = osTimerNew(SystemStatus::fastTimeoutEvenCb, osTimerPeriodic, this, nullptr);
  timerOneOff = osTimerNew(SystemStatus::oneOffTimeoutEvenCb, osTimerOnce, this, nullptr);
}

/**
 * Checks if the timer is running and starts it if not.
 * @param timer
 */
void SystemStatus::startTImerIfNotRunning(TimerInstance timer)
{
  osTimerId_t timerId;
  uint32_t timeout;
  bool *timerActiveLock;

  if (timer == TimerInstance::TI_NORMAL)
  {
    timerId = timerNormal;
    timeout = 500;
    timerActiveLock = &normalTimerActive;
  }
  else
  {
    timerId = timerFast;
    timeout = 200;
    timerActiveLock = &fastTimerActive;
  }

  if (!*timerActiveLock)
  {
    *timerActiveLock = true;
    osTimerStart(timerId, timeout);
  }
}

/**
 * Raises a new error condition
 * @param error
 */
void SystemStatus::raiseError(ErrorStatus error)
{
  switch (error)
  {
    case ErrorStatus::ERS_AUDIO_FE_FAILED:
      feError = true;
      globalServices->getSystemController()->getGpio(GpioInterface::GPIO_LED_4)->set(0);
      break;

    case ErrorStatus::ERS_SDCARD_FAILED:
      sdcardFailed = true;

      startTImerIfNotRunning(TimerInstance::TI_FAST);
      break;

    default:
      // Both blinking very quickly
      break;
  }

  fastTimeoutEvent();
}

/**
 * Reports the normal operation status
 */
void SystemStatus::reportStatus(OperationalStatus status)
{
  if (status == OperationalStatus::OPS_AUDIO_VOL)
  {
    globalServices->getSystemController()->getGpio(GpioInterface::GPIO_LED_2)->set(0);
    osTimerStart(timerOneOff, 150);
  }
  else
  {
    opStatus = status;
    normalTimeoutEvent();
    startTImerIfNotRunning(TimerInstance::TI_NORMAL);
  }
}

/**
 * This is the timeout handler that manages the normal events
 */
void SystemStatus::normalTimeoutEvent()
{
  if (opStatus == OperationalStatus::OPS_AUDIO_PLAYBACK)
  {
    playbackLed = !playbackLed;
  }
  else if (opStatus == OperationalStatus::OPS_AUDIO_PAUSE)
  {
    playbackLed = false;
  }
  else
  {
    playbackLed = true;
  }

  globalServices->getSystemController()->getGpio(GpioInterface::GPIO_LED_1)->set(playbackLed);
}

/**
 * This is the timeout handler that manages the fast/urgent events
 */
void SystemStatus::fastTimeoutEvent()
{
  if (sdcardFailed)
  {
    globalServices->getSystemController()->getGpio(GpioInterface::GPIO_LED_3)->set(fastLed);
    fastLed = !fastLed;
  }
}

/**
 * This is the timeout handler that manages the fast/urgent events
 */
void SystemStatus::oneOffTimeoutEvent()
{
  globalServices->getSystemController()->getGpio(GpioInterface::GPIO_LED_2)->set(1);
}

extern "C" void SYSTEM_Task(void *argument)
{
  Services *globalServices = static_cast<Services*>(argument);
  globalServices->getSystemController()->init();

  while (1)
  {
    osDelay(-1);
  }
}

}
