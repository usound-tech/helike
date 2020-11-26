//====================================================================
//
// COPYRIGHT 2019 All rights reserved.
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
//  Description: Audio Out interface - SAI sink
//  Filename: audio_out_sai.c
//  Author(s): Kumar Bala (kb@socfpga.io)
//  Date: 20-November-2019
//
//====================================================================

#include "Interfaces/Dac/pub/Dac.hpp"
#include "Interfaces/AudioLocal/pub/AudioLocalIn.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/System/pub/SystemController.hpp"
#include "Controllers/System/pub/SystemStatus.hpp"
#include "Controllers/Audio/pub/AudioService.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "sai.h"


//TODO: Add input jitter buffer
//TODO: Add rate calculation on dma end

namespace PeripheralInterface
{

AudioLocalIn::AudioLocalIn(System::SystemBus systemBus) :
    frequency(48000),
    systemBus(systemBus)
{

}

AudioLocalIn::~AudioLocalIn()
{

}

/**
 * Initialises the SAI input
 */
void AudioLocalIn::init()
{
  uint32_t bufferTime = globalServices->getSystemConfiguration()->getBufferingTIme();
  chunkSizePerTransfer = bufferTime * 2 * 48;

  // We make the fifo 4x longer than the DMA time, to allow start and stop conditions.
  // We need to revise that, as it adds more latency
  audioBufferLength = chunkSizePerTransfer * 2;
  rxSamples = new uint16_t[audioBufferLength];
  memset(rxSamples, 0, audioBufferLength * sizeof(uint16_t));

  silenceSamples = new uint16_t[chunkSizePerTransfer];
  memset(silenceSamples, 0, chunkSizePerTransfer * sizeof(uint16_t));
}

void AudioLocalIn::deinit()
{
  auto systemController = globalServices->getSystemController();

  auto saiInBus = systemController->getBus(systemBus);
  saiInBus->disable();
}

void AudioLocalIn::doAction(System::Action action)
{
  switch (action)
  {
    case System::Action::ENABLE:
      globalServices->getSystemStatus()->reportStatus(System::OperationalStatus::OPS_AUDIO_PAUSE);
      enable();
      break;

    case System::Action::DISABLE:
      deinit();
      break;

    case System::Action::START:
      globalServices->getSystemStatus()->reportStatus(System::OperationalStatus::OPS_AUDIO_PLAYBACK);
      break;

    case System::Action::STOP:
      globalServices->getSystemStatus()->reportStatus(System::OperationalStatus::OPS_AUDIO_PAUSE);
      memset(rxSamples, 0, audioBufferLength * sizeof(uint16_t));
      break;

    default:
      throw System::PeripheralException("Unknown action for AudioSaiIn: " + action);
  }
}

/**
 * Starts the SAI dma and enables the SAI interface if it's disabled
 */
void AudioLocalIn::enable()
{
  auto systemController = globalServices->getSystemController();
  frequency = globalServices->getSystemConfiguration()->getSaiInterfaceConfiguration(System::SaiInterface::TWEETER)->frequency;

  auto saiInBus = systemController->getBus(systemBus);
  saiInBus->enable();
}

/**
 * Returns the number of available samples.
 * It cannot distinguish between a full buffer and an empty buffer - it'll return 0 samples in both cases.
 */
uint32_t AudioLocalIn::getAvailableSampleCount()
{
  if (wr >= rd)
  {
    return wr - rd;
  }
  else
  {
    return wr + audioBufferLength - rd;
  }
}

/**
 * Returns a pointer to the oldest received sample.
 */
uint16_t* AudioLocalIn::getData(uint32_t length)
{
  if (getAvailableSampleCount() < length)
  {
    return silenceSamples;
  }

  uint16_t *buffer = &rxSamples[rd];

  rd += length;
  if (rd >= audioBufferLength)
  {
    rd = 0;
  }

  return buffer;
}

void AudioLocalIn::consumedData(uint32_t length)
{
  //NOT SUPPORTED
}

/**
 * Notifies the audio source that there are more data to be processed
 */
void AudioLocalIn::notifyDataAvailable()
{
  auto systemController = globalServices->getSystemController();
  System::Bus *bus = systemController->getBus(systemBus);

  // The DMA actually runs twice as fast as the rest of the audio, so it has half the data to send
  uint16_t samplesToRead = chunkSizePerTransfer / (systemBus == System::SystemBus::USB ? 4 : 2);
  bus->read(0, 0, 0, (uint8_t*) &rxSamples[wr], &samplesToRead, 0);

  wr += samplesToRead;
  if (wr >= audioBufferLength)
  {
    wr = 0;
  }
}

/**
 * Returns the source frequency.
 * In this version only 48 kHz is supported.
 */
uint32_t AudioLocalIn::getFrequency()
{
  return frequency;
}

bool AudioLocalIn::skipNext()
{
  //NOT SUPPORTED
  return true;
}

bool AudioLocalIn::skipPrev()
{
  //NOT SUPPORTED
  return true;
}

}
