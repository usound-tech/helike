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
//  Description: Wav file reader
//  Filename: WavReader.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include <Controllers/FilePlayer/src/WavReader.hpp>

namespace Controller
{


WavReader::WavReader() :
    wavFile(nullptr),
    samples(0),
    channels(0)
{
}

bool WavReader::init(System::File *wavFile)
{
  uint8_t buffer[44];

  this->wavFile = wavFile;
  samplesConsumed = 0;

  wavFile->seek(0);
  wavFile->read(buffer, 44);

  if (!(buffer[0] == 'R'
      && buffer[1] == 'I'
      && buffer[2] == 'F'
      && buffer[3] == 'F'))
  {
    return false;
  }

  if (!(buffer[8] == 'W'
      && buffer[9] == 'A'
      && buffer[10] == 'V'
      && buffer[11] == 'E'
      && buffer[12] == 'f'
      && buffer[13] == 'm'
      && buffer[14] == 't'
      && buffer[15] == 0x20))
  {
    return false;
  }

  if (!(buffer[36] == 'd'
      && buffer[37] == 'a'
      && buffer[38] == 't'
      && buffer[39] == 'a'))
  {
    return false;
  }

  // AudioFormat: 1 - PCM is the only supported format
  if (!(buffer[20] == 0x01 && buffer[21] == 0x00))
  {
    return false;
  }

  // BitsPerSample: 16 is the only supported value
  if (!(buffer[34] == 0x10 && buffer[35] == 0x00))
  {
    return false;
  }

  samples = *((uint32_t*) &buffer[40]) / sizeof(int16_t);
  channels = *((uint16_t*) &buffer[22]);

  return true;
}

/**
 * Returns true if the file suffix is .wav
 * @param filename
 * @return
 */
bool WavReader::isAKnownFile(std::string &filename)
{
  return (filename.size() > 4
      && filename[filename.size() - 4] == '.'
      && std::tolower(filename[filename.size() - 3]) == 'w'
      && std::tolower(filename[filename.size() - 2]) == 'a'
      && std::tolower(filename[filename.size() - 1]) == 'v');
}

/**
 * Reads the requested amount of data from the sdcard file
 * @param buffer
 * @param sampleCount number of samples (for all channels) to fetch from the file
 * @return
 */
bool WavReader::loadNextChunk(uint8_t *buffer, uint32_t sampleCount)
{
  samplesConsumed += (sampleCount / channels);

  sampleCount *= sizeof(int16_t);

  return wavFile->read(buffer, sampleCount) == sampleCount;
}

uint32_t WavReader::getFrequency()
{
  //TODO: Read this from the wav header
  return 48000;
}

void WavReader::getPlaybackInfo(uint32_t &trackTime, uint32_t &playbackTime)
{
  trackTime = 0;
  playbackTime = 0;

  trackTime = (samples / (48000 * channels));
  playbackTime = samplesConsumed / 48000;
}


}

