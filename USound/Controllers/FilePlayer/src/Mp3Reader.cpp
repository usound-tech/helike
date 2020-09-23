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
//  Description: Mp3 file reader
//  Filename: Mp3Reader.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include <Controllers/FilePlayer/src/Mp3Reader.hpp>
#include <string.h>

namespace Controller
{
#define MP3_HEADER_SIZE_POSITION  0x06

/* Supported VBR types */
#define VBR_Xing                  1
#define VBR_Info                  2
#define VBR_VBRI                  3


Mp3Reader::Mp3Reader()
{
  pcmBuffer = (int32_t*) new int16_t[(1152 << 1)];
}

/**
 * Callback function to ask for more compressed data
 *
 * @param  pCompressedData: pointer to the target buffer to be filled.
 * @param  nDataSizeInChars: number of data to be read in bytes
 * @param  pUserData: optional parameter
 * @retval return the decode frame.
 */
unsigned int Mp3Reader::readDataCb(void *compressedData, unsigned int byteCount, void *pUserData)
{
  Mp3Reader *mp3Reader = static_cast<Mp3Reader*>(pUserData);
  return mp3Reader->fetchMoreCompressedData(compressedData, byteCount);
}

/**
 * Sets the position of the playback stream.
 * @param pos
 * @return
 */
uint32_t Mp3Reader::setPosition(uint32_t pos)
{
  mp3File->seek(pos);
  return 0;
}

/**
 * Initialises the decoder library for the next mp3
 *
 * @param  pHeader: pointer to the audio file header tab.
 * @param  pReadCallback: Read callback function.
 * @param  pSetPosCallback: Set position callback function
 * @retval true if the stream is ok, false otherwise.
 */
bool Mp3Reader::init(System::File *mp3File)
{
  volatile uint16_t time_out = 1153;
  uint8_t mp3Header[16];
  uint8_t tmpBuffer[10];
  uint8_t nb_frame = 0;

  samplesConsumed = 0;

  this->mp3File = mp3File;
  mp3File->read(mp3Header, 16);

  memset(&mp3Info, 0, sizeof(TSpiritMP3Info));

  // check for headers
  memcpy(tmpBuffer, mp3Header, 10);

  if (tmpBuffer[0] == 'I' && tmpBuffer[1] == 'D' && tmpBuffer[2] == '3')
  {
    /* check for data start offset. */
    rawDataOffset = ((tmpBuffer[MP3_HEADER_SIZE_POSITION] & 0x7f) << 21)
        | ((tmpBuffer[MP3_HEADER_SIZE_POSITION + 1] & 0x7f) << 14)
        | ((tmpBuffer[MP3_HEADER_SIZE_POSITION + 2] & 0x7f) << 7)
        | (tmpBuffer[MP3_HEADER_SIZE_POSITION + 3] & 0x7f);
  }
  else
  {
    rawDataOffset = 0;
  }

  SpiritMP3DecoderInit(&MP3Decoder_Instance, readDataCb, NULL, this);
  setPosition(rawDataOffset);

  /* MP3 decoder needs to process more than one frame to be sure that headers are correctly decoded */
  do
  {
    SpiritMP3Decode(&MP3Decoder_Instance, (short*) pcmBuffer, 1152, &mp3Info);

    /* check for two good frames */
    if (!(!(mp3Info.IsGoodStream) || (mp3Info.nBitrateKbps == 0) || (mp3Info.nSampleRateHz == 0)))
    {
      nb_frame++;
    }
  }
  while ((nb_frame < 1) && (time_out-- != 0));

  setPosition(rawDataOffset);
  fetchMoreCompressedData(pcmBuffer, ((1152 << 1) * sizeof(int16_t)));

  if (checkForVBR((uint8_t*) pcmBuffer, ((1152 << 1) * sizeof(int16_t))) != 0)
  {
    VBR_Detect = LastnBitrateKbps = 0x00;
  }

  memset(&MP3Decoder_Instance, 0, sizeof(TSpiritMP3Decoder));

  SpiritMP3DecoderInit(&MP3Decoder_Instance, readDataCb, NULL, this);    // Re-initialize the decoder
  setPosition(rawDataOffset);

  return (mp3Info.IsGoodStream != 0);
}

/**
 * This callback pushes more compressed data to the mp3 decoder
 *
 * @param compressedData the pointer to where to copy the data
 * @param byteCount
 * @return
 */
unsigned int Mp3Reader::fetchMoreCompressedData(void *compressedData, unsigned int byteCount)
{
  uint32_t chunkSize;
  uint32_t offset = 0;
  unsigned int res;
  unsigned int bytesRead = 0;

  while (byteCount)
  {
    chunkSize = (byteCount >= 512) ? 511 : (uint32_t) byteCount;
    byteCount -= chunkSize;

    res = mp3File->read(&((uint8_t*) compressedData)[offset], chunkSize);

    bytesRead += res;
    offset += chunkSize;

    if (res == 0)
    {
      break;
    }
  }

  return bytesRead;
}

/**
 * Returns true if the file suffix is .mp3
 * @param filename
 * @return
 */
bool Mp3Reader::isAKnownFile(std::string &filename)
{
  return (filename.size() > 4
      && filename[filename.size() - 4] == '.'
      && std::tolower(filename[filename.size() - 3]) == 'm'
      && std::tolower(filename[filename.size() - 2]) == 'p'
      && filename[filename.size() - 1] == '3');
}

/**
 * Reads the requested amount of data from the sdcard file
 * @param buffer
 * @param sampleCount
 * @return
 */
bool Mp3Reader::loadNextChunk(uint8_t *buffer, uint32_t sampleCount)
{
  sampleCount /= 2;

  samplesConsumed += sampleCount;

  return SpiritMP3Decode((TSpiritMP3Decoder*) &MP3Decoder_Instance, (int16_t*) buffer, sampleCount, &mp3Info) == sampleCount;
}

/**
 * Returns the configured operating frequency (in Hz)
 * @return
 */
uint32_t Mp3Reader::getFrequency()
{
  return mp3Info.nSampleRateHz;
}

void Mp3Reader::getPlaybackInfo(uint32_t &trackTime, uint32_t &playbackTime)
{
  trackTime = (mp3Info.nSamplesPerFrame * NumberOfFrames / mp3Info.nSampleRateHz);
  playbackTime = samplesConsumed / mp3Info.nSampleRateHz;
}

/**
 * Parse the MP3 file header and checks if VBR is valid
 *
 * @param  pHeader: pointer to the wave file header table.
 * @param  NbData: number of data to be parsed.
 * @retval 0 if VBR is valid, !0 else.
 */
uint32_t Mp3Reader::checkForVBR(uint8_t *pHeader, uint32_t NbData)
{
  uint32_t herderindex = 0, VBRheader = 0;

  NumberOfFrames = 0;
  NumberOfBytes = 0;

  do
  {
    if (pHeader[herderindex] == 'X')
    {
      if ((pHeader[herderindex] == 'X') && (pHeader[herderindex + 1] == 'i') &&
          (pHeader[herderindex + 2] == 'n') && (pHeader[herderindex + 3] == 'g'))
      {
        VBRheader = VBR_Xing;
        break;
      }
    }
    else if (pHeader[herderindex] == 'I')
    {
      if ((pHeader[herderindex] == 'I') && (pHeader[herderindex + 1] == 'n') &&
          (pHeader[herderindex + 2] == 'f') && (pHeader[herderindex + 3] == 'o'))
      {
        VBRheader = VBR_Info;
        break;
      }
    }
    else if (pHeader[herderindex] == 'V')
    {
      if ((pHeader[herderindex] == 'V') && (pHeader[herderindex + 1] == 'B') &&
          (pHeader[herderindex + 2] == 'R') && (pHeader[herderindex + 3] == 'I'))
      {
        VBRheader = VBR_VBRI;
        break;
      }
    }
    herderindex++;
  } while (herderindex < NbData);

  /* Check for VBR header ID in 4 ASCII chars, either 'Xing' or 'Info' */
  if ((VBRheader == VBR_Xing) || (VBRheader == VBR_Info))
  {
    NumberOfFrames = ((pHeader[herderindex + 8] << 24)
        | (pHeader[herderindex + 9] << 16)
        | (pHeader[herderindex + 10] << 8)
        | pHeader[herderindex + 11]);

    NumberOfBytes = ((pHeader[herderindex + 12] << 24)
        | (pHeader[herderindex + 13] << 16)
        | (pHeader[herderindex + 14] << 8)
        | pHeader[herderindex + 15]);

    VBR_Detect = 1;
    return 0;
  }

  /* Check for VBRI header ID in 4 ASCII chars, always 'VBRI' */
  if (VBRheader == VBR_VBRI)
  {
    NumberOfBytes = ((pHeader[herderindex + 10] << 24) | (pHeader[herderindex + 11] << 16) |
        (pHeader[herderindex + 12] << 8) | pHeader[herderindex + 13]);

    NumberOfFrames = ((pHeader[herderindex + 14] << 24) | (pHeader[herderindex + 15] << 16) |
        (pHeader[herderindex + 16] << 8) | pHeader[herderindex + 17]);
    VBR_Detect = 1;
    return 0;
  }

  return 1;
}

}

