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
//  Description: Audio player. It uses different media readers to load mp3 and wav files
//  Filename: AudioPlayer.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/System/pub/ModuleConfig.hpp"
#include <Controllers/FilePlayer/pub/AudioPlayer.hpp>
#include <Controllers/FilePlayer/src/Mp3Reader.hpp>
#include <Controllers/FilePlayer/src/WavReader.hpp>
#include "Controllers/System/pub/SystemController.hpp"
#include "Controllers/System/pub/SystemStatus.hpp"
#include "Interfaces/pub/SystemControl.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "OAL/pub/Oal.hpp"
#include "cmsis_os2.h"
#include <string.h>
#include <stdlib.h>


namespace Controller
{

enum AudioPlayerCommand
{
  AP_CMD_START,
  AP_CMD_STOP,
  AP_CMD_DECODE_MORE,
  AP_CMD_NEXT,
  AP_CMD_PREV
};

struct __attribute__((packed)) Mp3PlayerCmd
{
  uint8_t cmd;
  uint8_t res;
  uint16_t arg;
};



/**
 * The entry point of the OS thread
 * @param argument
 */
void AudioPlayer::taskEntry(void *argument)
{
  AudioPlayer *mp3Player = static_cast<AudioPlayer*>(argument);
  mp3Player->taskLoop();
}

/**
 * Mp3 player constructor
 */
AudioPlayer::AudioPlayer() :
    filesystemListHandle(nullptr),
    controlMessageQueue(nullptr),
    audioSsamples(nullptr),
    audioState(AudioPlayerState::AP_UNCONFIGURED),
    audioFile(nullptr),
    activeReader(nullptr)
{

}

/**
 * Initialises the mp3 player
 */
void AudioPlayer::init()
{
  uint32_t bufferTime = globalServices->getSystemConfiguration()->getBufferingTIme();
  audioSsamples = new int16_t[bufferTime * 2 * 48];

  silenceAudioSamples();

#if MP3_READER_MODULE_ENABLED == 1
  fileReaders[AudioFileReader::MP3_READER] = new Mp3Reader();
#else
  fileReaders[AudioFileReader::MP3_READER] = nullptr;
#endif

  fileReaders[AudioFileReader::WAV_READER] = new WavReader();

  audioFile = globalServices->getFilesystem()->getFile();

  auto oal = globalServices->getOal();
  controlMessageQueue = oal->createMessageQueue(4, sizeof(Mp3PlayerCmd));
  oal->startTask((char*) "audioPlayer", 1024, System::OalTaskPriority::PRIO_MEDIUM, AudioPlayer::taskEntry, (void*) this);
}

/**
 * Returns the current data pointer
 * @param data
 * @param length
 */
uint16_t* AudioPlayer::getData(uint32_t length)
{
  if (audioState == AudioPlayerState::AP_ERROR)
  {
    return nullptr;
  }

  return (uint16_t*) audioSsamples;
}

/**
 * Triggers the mp3 player to decode more data
 * @param length number of samples to generate (for all channels)
 */
void AudioPlayer::consumedData(uint32_t length)
{
  Mp3PlayerCmd cmd = { AP_CMD_DECODE_MORE, 0, (uint16_t) length };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
}

/**
 * Returns the configured operating frequency (in Hz)
 * @return
 */
uint32_t AudioPlayer::getFrequency()
{
  if (activeReader)
  {
    return activeReader->getFrequency();
  }

  return 48000;
}

/**
 * Notifies the audio source that there are more data to be processed
 */
void AudioPlayer::notifyDataAvailable()
{
  //UNSUPPORTED
}

/**
 * Zeroes out the audio samples buffer
 */
void AudioPlayer::silenceAudioSamples()
{
  uint32_t bufferTime = globalServices->getSystemConfiguration()->getBufferingTIme();
  memset(audioSsamples, 0, bufferTime * 2 * 48 * sizeof(int16_t));
}

/**
 * Instructs the mp3 player to play the next track
 * @return
 */
bool AudioPlayer::skipNext()
{
  silenceAudioSamples();

  Mp3PlayerCmd cmd = { AP_CMD_NEXT, 0, 0 };
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);

  return true;
}

/**
 * Instructs the mp3 player to play the previous track
 * @return
 */
bool AudioPlayer::skipPrev()
{
//TODO: Implement prev
  return false;
}

/**
 * Opens the next audio file and detects the appropriate reader.
 */
bool AudioPlayer::playFile(bool advanceNext)
{
  auto fs = globalServices->getFilesystem();
  MediaFileReader *reader = nullptr;

  if (!filesystemListHandle)
  {
    filesystemListHandle = fs->startList(nullptr);
    if (!filesystemListHandle)
    {
      audioState = AudioPlayerState::AP_ERROR;
      return false;
    }

    advanceNext = true;
  }

  if (advanceNext)
  {
    uint32_t retries = 2;

    while (retries)
    {
      filename.clear();
      activeReader = nullptr;

      const char *fname = fs->getNextListFileName(filesystemListHandle);
      if (fname)
      {
        filename.append(fname);

        if (fileReaders[AudioFileReader::MP3_READER] != nullptr && fileReaders[AudioFileReader::MP3_READER]->isAKnownFile(filename))
        {
          reader = fileReaders[AudioFileReader::MP3_READER];
          retries = 0;
        }
        else if (fileReaders[AudioFileReader::WAV_READER] != nullptr && fileReaders[AudioFileReader::WAV_READER]->isAKnownFile(filename))
        {
          reader = fileReaders[AudioFileReader::WAV_READER];
          retries = 0;
        }
      }
      else
      {
        // Restart the directory list from the top
        if (!fs->startList(filesystemListHandle))
        {
          audioState = AudioPlayerState::AP_ERROR;
          return false;
        }
        retries--;
      }
    }
  }
  else
  {
    reader = activeReader;
  }

  if (!reader || filename.empty() || !audioFile->open(filename.c_str(), true))
  {
    return false;
  }

  if (!reader->init(audioFile))
  {
    audioFile->close();
    return false;
  }

  activeReader = reader;
  return true;
}

/**
 * Starts/resumes playing from filelist
 */
void AudioPlayer::play()
{
  if (audioState == AudioPlayerState::AP_PLAYING)
  {
    return;
  }

  if (!playFile(false))
  {
    return;
  }

  globalServices->getSystemStatus()->reportStatus(System::OperationalStatus::OPS_AUDIO_PLAYBACK);

  audioState = AudioPlayerState::AP_PLAYING;
}

/**
 * Stops the playback
 */
void AudioPlayer::pause()
{
  if (audioState != AudioPlayerState::AP_PLAYING)
  {
    return;
  }

  globalServices->getSystemStatus()->reportStatus(System::OperationalStatus::OPS_AUDIO_PAUSE);

  audioFile->close();
  audioState = AudioPlayerState::AP_IDLE;
}


void AudioPlayer::taskLoop()
{
  audioState = AudioPlayerState::AP_IDLE;

  auto oal = globalServices->getOal();
  Mp3PlayerCmd cmd;

  while (1)
  {
    if (!oal->popMessageFromQueue(controlMessageQueue, &cmd, osWaitForever))
    {
      continue;
    }

    switch (cmd.cmd)
    {
      case AP_CMD_START:
        play();
        break;

      case AP_CMD_STOP:
        pause();
        break;

      case AP_CMD_DECODE_MORE:
        if (activeReader)
        {
          if (!activeReader->loadNextChunk((uint8_t*) audioSsamples, (uint32_t) cmd.arg))
          {
            skipNext();
          }
        }

        break;

      case AP_CMD_NEXT:
        activeReader = nullptr;
        silenceAudioSamples();

        if (!playFile(true))
        {
          if (audioState != AudioPlayerState::AP_ERROR)
          {
            skipNext();
          }
          else
          {
            globalServices->getSystemStatus()->reportStatus(System::OperationalStatus::OPS_NONE);
          }
        }
        break;

      case AP_CMD_PREV:
        break;
    }
  }
}

/**
 * Closes the mp3 player task
 */
void AudioPlayer::deinit()
{
//Not supported
}

/**
 * Performs a peripheral-level task
 * @param action
 */
void AudioPlayer::doAction(System::Action action)
{
  switch (action)
  {
    case System::Action::START:
      {
      Mp3PlayerCmd cmd = { AP_CMD_START, 0, 0 };
      globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
    }
      break;

    case System::Action::STOP:
      {
      Mp3PlayerCmd cmd = { AP_CMD_STOP, 0, 0 };
      globalServices->getOal()->sendMessageToQueue(controlMessageQueue, &cmd, 0);
    }
      break;

    default:
      break;
  }
}

void AudioPlayer::getPlaybackInfo(uint32_t &trackTime, uint32_t &playbackTime)
{
  if ((audioState != AudioPlayerState::AP_PLAYING) || !activeReader)
  {
    trackTime = playbackTime = 0;
    return;
  }

  activeReader->getPlaybackInfo(trackTime, playbackTime);
}

}
