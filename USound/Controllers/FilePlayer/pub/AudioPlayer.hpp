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
//  Description: Audio player
//  Filename: AudioPlayer.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Interfaces/pub/SystemControl.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/Filesystem/pub/Filesystem.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"


namespace Controller
{

/**
 * This interface defines the common functions for all the media reader classes
 */
class MediaFileReader
{
public:
  virtual bool init(System::File *wavFile) = 0;
  virtual bool isAKnownFile(std::string &filename) = 0;
  virtual bool loadNextChunk(uint8_t *buffer, uint32_t byteCount) = 0;
  virtual uint32_t getFrequency() = 0;
  virtual void getPlaybackInfo(uint32_t &trackTime, uint32_t &playbackTime) = 0;

  virtual ~MediaFileReader()
  {

  }
};

enum AudioFileReader
{
  MP3_READER,
  WAV_READER,
  MAX_READERS
};

enum AudioPlayerState
{
  AP_UNCONFIGURED,
  AP_IDLE,
  AP_PLAYING,
  AP_ERROR
};

class AudioPlayer: public GlobalServiceConsumer, public System::AudioSource<uint16_t>
{
private:
  void *filesystemListHandle;
  void *controlMessageQueue;
  int16_t *audioSsamples;
  AudioPlayerState audioState;
  System::File *audioFile;
  std::string filename;
  MediaFileReader *fileReaders[AudioFileReader::MAX_READERS];
  MediaFileReader *activeReader;

private:
  static void taskEntry(void *argument);

  void taskLoop();
  void play();
  void pause();
  bool playFile(bool advanceNext);
  void silenceAudioSamples();

public:
  AudioPlayer();

  uint16_t* getData(uint32_t length) override;
  void consumedData(uint32_t length) override;
  uint32_t getFrequency() override;
  void notifyDataAvailable() override;

  bool skipNext() override;
  bool skipPrev() override;

  void init() override;
  void deinit() override;
  void doAction(System::Action action) override;
  void getPlaybackInfo(uint32_t &trackTime, uint32_t &playbackTime);

  AudioPlayerState getAudioState()
  {
    return audioState;
  }

  std::string& getFilename()
  {
    return filename;
  }
};

}
