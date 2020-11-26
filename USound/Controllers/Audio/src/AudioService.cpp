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
//  Filename: AudioService.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "cmsis_os.h"
#include "../pub/AudioService.hpp"
#include "OAL/pub/Oal.hpp"
#include "Controllers/System/pub/SystemController.hpp"
#include "Controllers/System/pub/SystemInterfaces.hpp"
#include "Controllers/System/pub/SystemStatus.hpp"
#include "Interfaces/pub/SystemControl.hpp"
#include "AudioFilters.hpp"
#include "Drc.hpp"

namespace System
{

enum AudioCommand
{
  CMD_TOGGLE_PLAY,
  CMD_VOL_UP_DOWN,
  CMD_SET_VOL,
  CMD_SET_MUTE,
  CMD_SKIP_PREV,
  CMD_SKIP_NEXT,
  CMD_RECONF_FILTERS,
  CMD_COUNT
};

struct __attribute__((packed)) AudioServiceCmd
{
  uint8_t cmd;
  uint8_t res;
  uint16_t arg;
};

/**
 * The entry point of the OS thread for the audio control task
 * @param argument
 */
void AudioService::taskControlEntry(void *argument)
{
  AudioService *audioService = static_cast<AudioService*>(argument);
  audioService->taskControlLoop();
}

/**
 * The entry point of the OS thread for the audio data out processing task
 * @param argument
 */
void AudioService::taskDataOutEntry(void *argument)
{
  AudioService *audioService = static_cast<AudioService*>(argument);
  audioService->taskDataOutLoop();
}

/**
 * The entry point of the OS thread for the audio data in processing task
 * @param argument
 */
void AudioService::taskDataInEntry(void *argument)
{
  AudioService *audioService = static_cast<AudioService*>(argument);
  audioService->taskDataInLoop();
}

void AudioService::timeoutEventCb(void *arg)
{
  AudioService *audioService = static_cast<AudioService*>(arg);
  audioService->timeoutEvent();
}

/**
 * Hid controller constructor
 */
AudioService::AudioService() :
    audioGain(0xF0)
{

}

void AudioService::init()
{
  System::SystemController *systemController = globalServices->getSystemController();
  audioMode = globalServices->getSystemConfiguration()->getAudioMode();

  initFilters();

  if ((audioMode == AudioMode::AM_MP3) || (audioMode == AudioMode::AM_I2S_SLAVE))
  {
    systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_NORTH)->setEventHandler([this](uint32_t gpioLevel, GpioEventType evType)
        { this->volUpDown(evType == GpioEventType::GEVT_SINGLE ? 2 : 8, AudioChangeSrc::ACS_JOYSTICK);});

    systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_SOUTH)->setEventHandler([this](uint32_t gpioLevel, GpioEventType evType)
        { this->volUpDown(evType == GpioEventType::GEVT_SINGLE ? -2 : -8, AudioChangeSrc::ACS_JOYSTICK);});
  }

  auto oal = globalServices->getOal();
  controlMessageQueue = oal->createMessageQueue(8, sizeof(AudioServiceCmd));
  oal->startTask((char*) "audio_service", 1024, System::OalTaskPriority::PRIO_MEDIUM, AudioService::taskControlEntry, (void*) this);
  oal->startTask((char*) "audio_data_out", 1024, System::OalTaskPriority::PRIO_EXTREME, AudioService::taskDataOutEntry, (void*) this);

  if ((audioMode == AudioMode::AM_I2S_SLAVE) || (audioMode == AudioMode::AM_USB))
  {
    oal->startTask((char*) "audio_data_in", 1024, System::OalTaskPriority::PRIO_EXTREME, AudioService::taskDataInEntry, (void*) this);
    detectI2sStop = osTimerNew(AudioService::timeoutEventCb, osTimerOnce, this, nullptr);
  }
  else if (audioMode == AudioMode::AM_MP3)
  {
    systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_CENTER)->setEventHandler([this](uint32_t gpioLevel, GpioEventType evType)
        { this->togglePlay(AudioChangeSrc::ACS_JOYSTICK);});

    systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_EAST)->setEventHandler([this](uint32_t gpioLevel, GpioEventType evType)
        { this->nextTrack(AudioChangeSrc::ACS_JOYSTICK);});
  }
}

/**
 * Initialises the biquad filter chains and the Dynamic Range Compressors
 */
void AudioService::initFilters()
{
  auto systemConfig = globalServices->getSystemConfiguration();
  uint32_t bufferingTime = systemConfig->getBufferingTIme();

  audioFilters = new AudioFilters(systemConfig->getFilterConfiguration(), 48 * bufferingTime);
  audioFilters->init();
}

void AudioService::startPlay(AudioChangeSrc acs)
{
  audioActive = false;
  togglePlay(acs);
}

void AudioService::stopPlay(AudioChangeSrc acs)
{
  audioActive = true;
  togglePlay(acs);
}

/**
 * Checks if the Audio Engine mode should accept an audio change command from the provided source.
 * @param acs
 * @return
 */
bool AudioService::isAudioCommandSupportedInCurrentMode(AudioChangeSrc acs)
{
  if (audioMode == AudioMode::AM_USB)
  {
    if (acs == AudioChangeSrc::ACS_JOYSTICK)
    {
      return false;
    }
  }
  else if (acs == AudioChangeSrc::ACS_USB)
  {
    return false;
  }

  return true;
}

void AudioService::togglePlay(AudioChangeSrc acs)
{
  if (!audioSrc || !audioSink || !isAudioCommandSupportedInCurrentMode(acs))
  {
    return;
  }

  AudioServiceCmd cmd = { CMD_TOGGLE_PLAY, 0, 0 };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
}

void AudioService::volUpDown(int32_t level, AudioChangeSrc acs)
{
  if (!isAudioCommandSupportedInCurrentMode(acs))
  {
    return;
  }

  AudioServiceCmd cmd = { CMD_VOL_UP_DOWN, 0, (uint16_t) level };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
}

void AudioService::setVolume(uint32_t level, AudioChangeSrc acs)
{
  if (!isAudioCommandSupportedInCurrentMode(acs))
  {
    return;
  }

  AudioServiceCmd cmd = { CMD_SET_VOL, 0, (uint16_t) level };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
}

void AudioService::setMute(bool mute, AudioChangeSrc acs)
{
  if (!isAudioCommandSupportedInCurrentMode(acs))
  {
    return;
  }

  AudioServiceCmd cmd = { CMD_SET_MUTE, 0, (uint16_t) mute };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
}

void AudioService::nextTrack(AudioChangeSrc acs)
{
  if (!isAudioCommandSupportedInCurrentMode(acs))
  {
    return;
  }

  AudioServiceCmd cmd = { CMD_SKIP_NEXT, 0, 0 };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
}

void AudioService::reconfigureFilters()
{
  AudioServiceCmd cmd = { CMD_RECONF_FILTERS, 0, 0 };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
}

void AudioService::prevTrack(AudioChangeSrc acs)
{
//NOT SUPPORTED
}

/**
 * This is the audio control loop that handles the audio playback and volume commands
 */
void AudioService::taskControlLoop()
{
  auto oal = globalServices->getOal();
  AudioServiceCmd cmd;
  TickType_t lastActionTs = 0;
  TickType_t now;

  //This call triggers the mp3 autorun when Helike starts
  if (audioMode == AudioMode::AM_MP3)
  {
    startPlay(AudioChangeSrc::ACS_AUDIO_ENGINE);
  }

  while (1)
  {
    if (!oal->popMessageFromQueue(controlMessageQueue, &cmd, osWaitForever))
    {
      continue;
    }

    now = xTaskGetTickCount();
    if ((now - 200) < lastActionTs)
    {
      continue;
    }
    lastActionTs = now;

    switch (cmd.cmd)
    {
      case CMD_TOGGLE_PLAY:
        audioActive = !audioActive;

        audioSrc->doAction(audioActive ? Action::START : Action::STOP);
        audioSink->doAction(audioActive ? Action::START : Action::STOP);

        if (audioMode == AudioMode::AM_I2S_SLAVE && audioActive)
        {
          osTimerStart(detectI2sStop, 200);
        }

        break;

      case CMD_VOL_UP_DOWN:
        case CMD_SET_VOL:
        globalServices->getSystemStatus()->reportStatus(OperationalStatus::OPS_AUDIO_VOL);

        if (cmd.cmd == CMD_VOL_UP_DOWN)
        {
          audioGain += (int16_t) cmd.arg;
        }
        else
        {
          audioGain = (int16_t) cmd.arg;
        }

        if (audioGain < 0)
        {
          audioGain = 0;
        }
        else if (audioGain > 0xFF)
        {
          audioGain = 0xFF;
        }
        audioSink->setVolume((uint8_t) audioGain);
        break;

      case CMD_SET_MUTE:
        globalServices->getSystemStatus()->reportStatus(OperationalStatus::OPS_AUDIO_VOL);
        audioSink->mute((bool) cmd.arg);
        break;

      case CMD_SKIP_PREV:
        break;

      case CMD_SKIP_NEXT:
        audioSrc->skipNext();
        break;

      case CMD_RECONF_FILTERS:
        audioFilters->init();
        break;
    }
  }
}

/**
 * Triggers the audio data path to process more data
 */
void AudioService::notifyMoreDataNeeded()
{
  audioOutCycles++;

  auto oal = globalServices->getOal();
  oal->sendTaskNotification(rxTaskHandle);
  rxTaskHandle = nullptr;
}

/**
 * Triggers the audio data path to accept more data
 */
void AudioService::notifyMoreDataAvailable()
{
  if (txTaskHandle)
  {
    auto oal = globalServices->getOal();
    oal->sendTaskNotification(txTaskHandle);
    txTaskHandle = nullptr;
  }

  if (!audioActive)
  {
    startPlay(AudioChangeSrc::ACS_AUDIO_ENGINE);
  }
  else if (audioMode == AudioMode::AM_I2S_SLAVE)
  {
    osTimerReset(detectI2sStop);
  }
}

/**
 * This is the audio data out loop that handles the audio out processing
 */
void AudioService::taskDataOutLoop()
{
  auto oal = globalServices->getOal();
  uint32_t bufferingTime = globalServices->getSystemConfiguration()->getBufferingTIme();
  int16_t *scratchBuf[STREAM_ID::MAX_STREAM_COUNT];
  scratchBuf[STREAM_ID::STREAM_TWEETER] = new int16_t[bufferingTime * 48 * 2];
  scratchBuf[STREAM_ID::STREAM_WOOFER] = new int16_t[bufferingTime * 48 * 2];
  uint32_t len = 0;

  while (1)
  {
    rxTaskHandle = xTaskGetCurrentTaskHandle();
    oal->waitForTaskNotification(-1);

    if (audioSrc && audioSink)
    {
      //TODO: We are assuming that both source and sink run at the same frequency. We need to make this dynamic.
      if (!len)
      {
        uint32_t sinkFrequency = audioSink->getFrequency();
        len = sinkFrequency * bufferingTime * 2 / 1000;
      }

      int16_t *dataIn = (int16_t*) audioSrc->getData(len);
      if (dataIn)
      {
        audioFilters->run(dataIn, scratchBuf);

        audioSink->enqueueData((uint16_t*) scratchBuf[STREAM_ID::STREAM_TWEETER], len, (uint32_t) System::SaiInterface::TWEETER);
        audioSink->enqueueData((uint16_t*) scratchBuf[STREAM_ID::STREAM_WOOFER], len, (uint32_t) System::SaiInterface::WOOFER);

        audioSrc->consumedData(len);
      }
      else
      {
        stopPlay(AudioChangeSrc::ACS_AUDIO_ENGINE);
      }
    }
  }
}

/**
 * This is the audio data in loop that handles the audio data in processing
 */
void AudioService::taskDataInLoop()
{
  auto oal = globalServices->getOal();

  while (1)
  {
    txTaskHandle = xTaskGetCurrentTaskHandle();
    oal->waitForTaskNotification(-1);

    if (audioSrc)
    {
      audioSrc->notifyDataAvailable();
    }
  }
}

/**
 * Selects the audio source peripheral. If the mode is I2S slave, it also starts the slave interface
 * @param audioSrc
 */
SystemAudioSource AudioService::selectAudioSource(SystemAudioSource audioSrcInterface)
{
  SystemAudioSource prevSrc = systemAudioSrc;

  audioSrc = globalServices->getSystemController()->getAudioSource(audioSrcInterface);
  systemAudioSrc = audioSrcInterface;

  if (audioSrcInterface == SystemAudioSource::AUDIO_SRC_SAI)
  {
    audioSrc->doAction(Action::ENABLE);
  }

  return prevSrc;
}

/**
 * Selects the audio sink peripheral
 * @param audioSink
 */
SystemAudioSink AudioService::selectAudioSink(SystemAudioSink audioSinkInterface)
{
  SystemAudioSink prevSink = systemAudioSink;
  audioSink = globalServices->getSystemController()->getAudioSink(audioSinkInterface);
  systemAudioSink = audioSinkInterface;

  return prevSink;
}

/**
 * This function is invoked when the i2s audio detect timer expires,
 * which means that the master stopped sending audio data to Helike
 */
void AudioService::timeoutEvent()
{
  stopPlay(AudioChangeSrc::ACS_AUDIO_ENGINE);
}

/**
 * Returns the audio out cycles
 * @param resetCounter
 * @return
 */
uint32_t AudioService::getAudioOutCycles(bool resetCounter)
{
  uint32_t samples = audioOutCycles;

  if (resetCounter)
  {
    audioOutCycles = 0;
  }

  return samples;
}

}
