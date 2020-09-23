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
//  Description: System status is responsible for reporting the status
//               using the LEDs.
//  Filename: SystemStatus.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Interfaces/pub/SystemControl.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/Console/pub/Console.hpp"
#include "cmsis_os2.h"


namespace System
{

enum OperationalStatus
{
  OPS_NONE,
  OPS_AUDIO_PLAYBACK,
  OPS_AUDIO_PAUSE,
  OPS_AUDIO_VOL
};

enum ErrorStatus
{
  ERS_SDCARD_FAILED,
  ERS_AUDIO_FE_FAILED,
  ERS_UNSUPPORTED_CONFIG
};

enum TimerInstance
{
  TI_NORMAL,
  TI_FAST
};

/**
 * This class manages the LED signalling of the system status.
 */
class SystemStatus: public GlobalServiceConsumer
{
private:
  OperationalStatus opStatus = OperationalStatus::OPS_NONE;

  bool sdcardFailed = false;
  bool feError = false;

  bool playbackLed = false;
  bool sdcardLed = false;
  bool fastLed = false;
  bool normalTimerActive = false;
  bool fastTimerActive = false;

  osTimerId_t timerNormal = nullptr;
  osTimerId_t timerFast = nullptr;
  osTimerId_t timerOneOff = nullptr;

private:
  void normalTimeoutEvent();
  void fastTimeoutEvent();
  void oneOffTimeoutEvent();
  void startTImerIfNotRunning(TimerInstance timer);

public:
  SystemStatus();

  void init();

  void raiseError(ErrorStatus error);
  void reportStatus(OperationalStatus status);

  static void normalTimeoutEventCb(void *arg);
  static void fastTimeoutEvenCb(void *arg);
  static void oneOffTimeoutEvenCb(void *arg);
};

}
