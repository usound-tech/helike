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
//  Description: Bson reader. It supports double, string, int32, arrays and nested objects.
//  Filename: BsonReader.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/BsonReader.hpp"


BsonReader::BsonReader()
{
}

/**
 * Reads a number array (double precision) from BSON and converts it to a float32_t array.
 *
 * @param dst the destination array
 * @param data the beginning of the Bson element
 * @param maxCount the maximum amount of numbers to retrieve
 *
 * @return the number of values it retrieved
 */
uint32_t BsonReader::getFloatArray(float32_t *dst, const uint8_t *data, uint32_t maxCount)
{
  uint32_t count = 0;

  uint32_t off = sizeof(uint32_t);
  BsonElem arrayElem;
  double tmpDouble;

  uint32_t len;
  memcpy(&len, data, sizeof(uint32_t));

  while (((off + 1) < len) && (count < maxCount))
  {
    getField(&data[off], arrayElem);

    if (arrayElem.type == BSON_TYPE_NUMBER)
    {
      memcpy(&tmpDouble, arrayElem.data, 8);
      dst[count] = (float) tmpDouble;
    }
    else if (arrayElem.type == BSON_TYPE_INT32)
    {
      int32_t tmp;
      memcpy(&tmp, arrayElem.data, 4);
      dst[count] = (float) tmp;
    }

    off += arrayElem.elemLen;
    count++;
  }

  return count;
}

/**
 * Reads an integer tuple array (keys is an integer and value is also an integer) from BSON and converts it to a pair array.
 *
 * @param dst the destination pair array
 * @param data the beginning of the Bson element
 * @param maxCount the maximum amount of numbers to retrieve
 *
 * @return the number of values it retrieved
 */
uint32_t BsonReader::getIntPairArray(std::pair<uint8_t, uint8_t> *dst, const uint8_t *data, uint32_t maxCount)
{
  uint32_t count = 0;

  uint32_t off = sizeof(uint32_t);
  BsonElem arrayElem;

  uint32_t len;
  memcpy(&len, data, sizeof(uint32_t));

  while (((off + 1) < len) && (count < maxCount))
  {
    getField(&data[off], arrayElem);

    dst[count].first = (uint8_t)strtoul((const char*)arrayElem.name, nullptr, 0);

    if (arrayElem.type == BSON_TYPE_STRING)
    {
      dst[count].second = (uint8_t)strtoul((const char*)arrayElem.data, nullptr, 0);
    }
    else if (arrayElem.type == BSON_TYPE_INT32)
    {
      int32_t tmp;
      memcpy(&tmp, arrayElem.data, 4);

      dst[count].second = (uint8_t)tmp;
    }

    off += arrayElem.elemLen;
    count++;
  }

  return count;
}

/**
 * Retrieves the number of array elements in Bson.
 *
 * @param data the beginning of the Bson element
 */
uint32_t BsonReader::getArrayCount(const uint8_t *data)
{
  uint32_t count = 0;

  uint32_t off = sizeof(uint32_t);
  BsonElem arrayElem;

  uint32_t len;
  memcpy(&len, data, sizeof(uint32_t));

  while ((off + 1) < len)
  {
    getField(&data[off], arrayElem);

    count++;
    off += arrayElem.elemLen;
  }

  return count;
}

/**
 * Returns the current Bson field that begins at the provided data pointer
 * @param data the beginning of the Bson element
 * @param elem the element object to store the Bson element attributes
 */
void BsonReader::getField(const uint8_t *data, BsonElem &elem)
{
  elem.type = *data++;
  elem.name = data;

  uint32_t keyLen = (uint32_t) strlen((const char*) data) + 1;
  data += keyLen;
  elem.elemLen = keyLen + 1;

  switch (elem.type)
  {
    case BSON_TYPE_NUMBER:
      elem.dataLen = 8;
      break;

    case BSON_TYPE_STRING: {
      uint32_t sz;
      memcpy(&sz, data, sizeof(uint32_t));
      data += sizeof(uint32_t);
      elem.dataLen = sz;
      elem.elemLen += sizeof(uint32_t);
    }
      break;

    case BSON_TYPE_INT32:
      elem.dataLen = sizeof(int32_t);
      break;

    case BSON_TYPE_BOOLEAN:
      elem.dataLen = sizeof(char);
      break;

    case BSON_TYPE_OBJECT:
      case BSON_TYPE_ARRAY: {
      uint32_t sz;
      memcpy(&sz, data, sizeof(uint32_t));
      elem.dataLen = sz;
    }
      break;

    default:
      break;
  }

  elem.elemLen += elem.dataLen;
  elem.data = data;
}

/**
 * Searches for a field with the specified name.
 * It can be used recursively, to retrieve fields of nested objects.
 *
 * @param data the beginning of the Bson document
 * @param fieldName the name of the field to be located
 * @param elem the element object to hold the fount element attributes
 * @return true if the element was found, false otherwise
 */
bool BsonReader::findField(const uint8_t *data, const char *fieldName, BsonElem &elem)
{
  BsonElem searchElem;
  uint32_t len;
  memcpy(&len, data, sizeof(uint32_t));

  uint32_t off = sizeof(uint32_t);
  elem.data = nullptr;

  while ((off + 1) < len)
  {
    getField(&data[off], searchElem);

    if (strcmp((const char*) searchElem.name, fieldName) == 0)
    {
      elem = searchElem;
      return true;
    }

    off += searchElem.elemLen;
  }

  return false;
}
