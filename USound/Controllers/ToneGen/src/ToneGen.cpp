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
//  Description: Mp3 player
//  Filename: Mp3Player.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/System/pub/SystemController.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "../pub/ToneGen.hpp"
#include "OAL/pub/Oal.hpp"
#include "arm_math.h"
#include <cmath>

namespace Controller
{

/**
 * Tone generator constructor
 */
ToneGen::ToneGen() :
    data(nullptr),
    pattern(PATTERN_SINE),
    sineFreq(1000),
    currentOffset(0)
{

}

void ToneGen::init()
{
  uint32_t bufferTime = globalServices->getSystemConfiguration()->getBufferingTIme();

  //TODO: Allocate buffer
  data = new uint16_t[bufferTime * 2 * 48];
}

/**
 * Returns the current data pointer
 * @param data
 * @param length
 */
uint16_t* ToneGen::getData(uint32_t length)
{
  for (uint32_t i = 0; i < length; i += 2)
  {
    if (pattern == PatternType::PATTERN_SINE)
    {
      data[i] = (int32_t) (32767.0f * sinf(2.0f * PI * currentOffset * sineFreq / 48000.0f));
    }
    else if (pattern == PatternType::PATTERN_RAMP)
    {
      data[i] = currentOffset;
    }

    data[i + 1] = data[i];
    currentOffset++;
    if (currentOffset >= 48000)
    {
      currentOffset = 0;
    }
  }

  return data;
}

/**
 * Triggers the mp3 player to decode more data
 * @param length
 */
void ToneGen::consumedData(uint32_t length)
{
//Nothing to do here
}

/**
 * Returns the configured operating frequency (in Hz)
 * @return
 */
uint32_t ToneGen::getFrequency()
{
  return 48000;
}

/**
 * Notifies the audio source that there are more data to be processed
 */
void ToneGen::notifyDataAvailable()
{
  //UNSUPPORTED
}


/**
 * Closes the mp3 player task
 */
void ToneGen::deinit()
{
//Not supported
}

/**
 * Performs a peripheral-level task
 * @param action
 */
void ToneGen::doAction(System::Action action)
{
//Not supported
}

/**
 * Selects the generated output pattern
 * @param patternType
 */
void ToneGen::setPatternType(PatternType patternType)
{
  pattern = patternType;
}

/**
 * Sets the sineFrequency
 * @param freq
 */
void ToneGen::setSineFrequency(uint32_t freq)
{
  sineFreq = freq;
}


bool ToneGen::skipNext()
{
  return true;
}

bool ToneGen::skipPrev()
{
  return true;
}



}
