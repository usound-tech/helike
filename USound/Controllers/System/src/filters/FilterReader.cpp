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

#include "FilterReader.hpp"
#include <memory>
#include "Controllers/Filesystem/pub/Filesystem.hpp"
#include "Utilities/BsonReader/pub/BsonReader.hpp"
#include "Controllers/System/pub/ModuleConfig.hpp"

namespace System
{

FilterReader::FilterReader(System::FilterConfiguration *filterConfig) :
    filterConfig(filterConfig)
{

}

void FilterReader::configFilter(uint32_t configOption)
{
  auto fs = globalServices->getFilesystem();

  if (!fs->isMounted())
  {
    return;
  }

  uint32_t size;
  uint8_t attrs;

  std::string fileName = "/usound/config-" + std::to_string(configOption);
  fileName += ".bin";

  if (!fs->statFile(fileName.c_str(), &size, &attrs))
  {
    return;
  }

  std::unique_ptr<System::File> filterFile(globalServices->getFilesystem()->getFile());

  if (!filterFile->open(fileName.c_str(), false))
  {
    return;
  }

  uint8_t *tmpBuffer = new uint8_t[size];
  if (filterFile->read(tmpBuffer, size) == size)
  {
    extractConfig(tmpBuffer);
  }

  delete[] tmpBuffer;
}

void FilterReader::loadFloatArray(const uint8_t *data, const char *name, float32_t *coeffArray, uint32_t maxCount)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    bson.getFloatArray(coeffArray, arrayElem.data, maxCount);
  }
}

uint32_t FilterReader::getArraySize(const uint8_t *data, const char *name)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    return bson.getArrayCount(arrayElem.data);
  }

  return 0;
}

void FilterReader::loadInt(const uint8_t *data, const char *name, uint32_t *value)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    memcpy(value, arrayElem.data, sizeof(uint32_t));
  }
}

void FilterReader::loadFloat32(const uint8_t *data, const char *name, float32_t *value)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    if (arrayElem.type == BSON_TYPE_NUMBER)
    {
      double tmp;
      memcpy(&tmp, arrayElem.data, 8);
      *value = (float32_t) tmp;
    }
    else if (arrayElem.type == BSON_TYPE_INT32)
    {
      uint32_t tmp;
      memcpy(&tmp, arrayElem.data, 4);
      *value = (float32_t) tmp;
    }
  }
}

void FilterReader::loadString(const uint8_t *data, const char *name, std::string &value)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    value.append((const char*) arrayElem.data);
  }
}

void FilterReader::loadBool(const uint8_t *data, const char *name, bool *value)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    *value = arrayElem.data[0];
  }
}

void FilterReader::extractConfig(const uint8_t *data)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, "name", arrayElem))
  {
    filterConfig->name = (const char*) arrayElem.data;
  }

  if (bson.findField(data, "masterEqCoeffs", arrayElem))
  {
    loadFloatArray(arrayElem.data, "leftMasterEqCoefficients", filterConfig->masterEqCoeffs[System::MasterEqCoeffcientType::LEFT], MASTER_EQ_STAGES * 5);
    loadFloatArray(arrayElem.data, "rightMasterEqCoefficients", filterConfig->masterEqCoeffs[System::MasterEqCoeffcientType::RIGHT], MASTER_EQ_STAGES * 5);
  }

  if (bson.findField(data, "xoverEqCoeffs", arrayElem))
  {
    loadFloatArray(arrayElem.data, "xoverWooferLeft", filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::LEFT_WOOFER], MASTER_EQ_STAGES * 5);
    loadFloatArray(arrayElem.data, "xoverTweeterLeft", filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::LEFT_TWEETER], MASTER_EQ_STAGES * 5);
    loadFloatArray(arrayElem.data, "xoverWooferRight", filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::RIGHT_WOOFER], MASTER_EQ_STAGES * 5);
    loadFloatArray(arrayElem.data, "xoverTweeterRight", filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::RIGHT_TWEETER], MASTER_EQ_STAGES * 5);
  }

  extractDrcConfig(arrayElem.data, "levelerDrcConfig", filterConfig->levelerDrcConfig);
  extractDrcConfig(arrayElem.data, "limiterDrcConfig", filterConfig->limiterDrcConfig);

  loadBool(data, "masterEqEnabled", &filterConfig->masterEqEnabled);
  loadBool(data, "xoverEqEnabled", &filterConfig->xoverEqEnabled);

  loadBool(data, "levelerDrcEnabled", &filterConfig->levelerDrcConfig.enabled);
  loadBool(data, "limiterDrcEnabled", &filterConfig->limiterDrcConfig.enabled);

#if PREDISTORTION_MODULE_ENABLED == 1
  loadBool(data, "predistortionEnabled", &filterConfig->predistortionConfig.enabled);
  extractPredistortionConfig(data, "predistortionConfig", filterConfig->predistortionConfig);
#endif
}

void FilterReader::extractDrcConfig(const uint8_t *data, const char *name, System::DrcConfiguration &drcConfig)
{
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    loadFloat32(data, "attackDuration", &drcConfig.attackDuration);
    loadFloat32(data, "releaseDuration", &drcConfig.releaseDuration);
    loadFloat32(data, "compressionThresholdFullScaleDb", &drcConfig.compressionThresholdFullScaleDb);
    loadFloat32(data, "compressionRatio", &drcConfig.compressionRatio);
    loadFloat32(data, "sampleRateHz", &drcConfig.sampleRateHz);
    loadFloat32(data, "postGain", &drcConfig.postGain);
  }
}

void FilterReader::extractPredistortionConfig(const uint8_t *data, const char *name, System::PredistortionConfiguration &predistortionConfig)
{
#if PREDISTORTION_MODULE_ENABLED == 1
  BsonReader bson;
  BsonElem arrayElem;

  if (bson.findField(data, name, arrayElem))
  {
    loadFloatArray(arrayElem.data, "downsampling", predistortionConfig.coeffs[System::PredistortionCoeffcientType::DOWNSAMPLING], MASTER_EQ_STAGES * 5);
    loadFloatArray(arrayElem.data, "upsampling", predistortionConfig.coeffs[System::PredistortionCoeffcientType::UPSAMPLING], MASTER_EQ_STAGES * 5);

    uint32_t rangeSize = getArraySize(arrayElem.data, "inputRange");
    predistortionConfig.rangeSize = rangeSize;

    if (rangeSize)
    {
      if (predistortionConfig.inputRange)
      {
        delete[] predistortionConfig.inputRange;
      }

      if (predistortionConfig.outputRange)
      {
        delete[] predistortionConfig.outputRange;
      }

      predistortionConfig.inputRange = new float32_t[rangeSize];
      predistortionConfig.outputRange = new float32_t[rangeSize];

      loadFloatArray(arrayElem.data, "inputRange", predistortionConfig.inputRange, rangeSize);
      loadFloatArray(arrayElem.data, "outputRange", predistortionConfig.outputRange, rangeSize);
    }
  }
#endif
}

}
