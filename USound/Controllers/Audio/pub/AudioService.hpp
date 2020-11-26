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
//  Description: Audio service
//  Filename: AudioService.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "cmsis_os2.h"

class AudioFilters;
class Drc;

namespace System
{

enum AudioChangeSrc
{
  ACS_JOYSTICK,
  ACS_USB,
  ACS_AUDIO_ENGINE
};

/**
 * This service is responsible for the control and data plane of the system audio
 */
class AudioService: public GlobalServiceConsumer
{
private:
  bool audioActive = false;
  int32_t audioGain;
  void *controlMessageQueue = nullptr;
  volatile void *rxTaskHandle = nullptr;
  volatile void *txTaskHandle = nullptr;

  AudioSource<uint16_t> *audioSrc = nullptr;
  AudioSink<uint16_t> *audioSink = nullptr;

  AudioFilters *audioFilters = nullptr;

  AudioMode audioMode = AudioMode::AM_MP3;
  osTimerId_t detectI2sStop = nullptr;      //!< This timer is used in I2S slave mode to detect that the host has stopped sending audio

  SystemAudioSource systemAudioSrc = SystemAudioSource::AUDIO_SRC_NONE;
  SystemAudioSink systemAudioSink = SystemAudioSink::AUDIO_SINK_NONE;

  uint32_t audioOutCycles = 0;

private:
  static void timeoutEventCb(void *arg);
  static void taskControlEntry(void *argument);

  void taskControlLoop();
  void taskDataOutLoop();
  void taskDataInLoop();
  void timeoutEvent();
  void initFilters();
  bool isAudioCommandSupportedInCurrentMode(AudioChangeSrc acs);

  static void taskDataOutEntry(void *argument);
  static void taskDataInEntry(void *argument);

public:
  AudioService();

  void init();

  void startPlay(AudioChangeSrc acs);
  void stopPlay(AudioChangeSrc acs);
  void togglePlay(AudioChangeSrc acs);
  void volUpDown(int32_t level, AudioChangeSrc acs);
  void setVolume(uint32_t level, AudioChangeSrc acs);
  void setMute(bool mute, AudioChangeSrc acs);
  void nextTrack(AudioChangeSrc acs);
  void prevTrack(AudioChangeSrc acs);
  void reconfigureFilters();

  void notifyMoreDataNeeded();
  void notifyMoreDataAvailable();

  SystemAudioSource selectAudioSource(SystemAudioSource audioSrc);
  SystemAudioSink selectAudioSink(SystemAudioSink audioSink);

  uint32_t getAudioOutCycles(bool resetCounter);
};

}
