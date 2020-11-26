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
//  Description: Filter reader. It retrieves the filter configuration from the sdcard (if present)
//  Filename: FilterReader.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include <stdint.h>

namespace System
{

/**
 * Helper class that reads Bson files from the sdcard and transforms them to filter configuration
 */
class FilterReader: public GlobalServiceConsumer
{
private:
  System::FilterConfiguration *filterConfig;

  void extractDrcConfig(const uint8_t *data, const char *name, System::DrcConfiguration &drcConfig);
  void extractPredistortionConfig(const uint8_t *data, const char *name, System::PredistortionConfiguration &predistortionConfig);

  uint32_t getArraySize(const uint8_t *data, const char *name);
  void loadFloatArray(const uint8_t *data, const char *name, float32_t *coeffArray, uint32_t maxCount);
  void loadInt(const uint8_t *data, const char *name, uint32_t *value);
  void loadString(const uint8_t *data, const char *name, std::string &value);
  void loadFloat32(const uint8_t *data, const char *name, float32_t *value);
  void loadBool(const uint8_t *data, const char *name, bool *value);

public:
  FilterReader(System::FilterConfiguration *filterConfig);

  void extractConfig(const uint8_t *data);
  void configFilter(uint32_t configOption);
};

}
