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
//  Filename: BsonReader.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================


#pragma once
#include <stdint.h>
#include <utility>
#include "arm_math.h"

#define BSON_DOC_SIZE    64  /* Max BSON Document size */
#define BSON_ELM_SIZE    64  /* Max BSON Element size  */
#define JSON_MAX_SIZE   128  /* Max JSON string size (BSONObject::jsonString) */

#define BSON_BUFF_SIZE   64  /* Max BSON Document length, increment this value as needed */
#define BSON_ELEM_SIZE   64  /* Max BSON Element length, increment this value as needed */

#define BSON_EOO        0x00 // \x00 EOO: End-Of-Object
#define BSON_NULL_BYTE  0x00 // \x00 NULL terminator
#define BSON_MINKEY     0xFF // \xFF Min key
#define BSON_MAXKEY     0x7F // \x7F Max key

#define BSON_TYPE_NUMBER        0x01 // \x01 double - 64-bit binary floating point
#define BSON_TYPE_STRING        0x02 // \x02 string - UTF-8 string
#define BSON_TYPE_OBJECT        0x03 // \x03 document - Embedded document
#define BSON_TYPE_ARRAY         0x04 // \x04 document - Array
#define BSON_TYPE_BINARY        0x05 // \x05 binary - Binary data
#define BSON_TYPE_UNDEFINED     0x06 // \x06 Deprecated
#define BSON_TYPE_OID           0x07 // \x07 (byte*12) - ObjectId
#define BSON_TYPE_BOOLEAN       0x08 // \x08
#define BSON_TYPE_DATE          0x09 // \x09
#define BSON_TYPE_NULL          0x0A // \x0A
#define BSON_TYPE_REGEX         0x0B // \x0B
#define BSON_TYPE_REF           0x0C // \x0C
#define BSON_TYPE_CODE          0x0D // \x0D
#define BSON_TYPE_SYMBOL        0x0E // \x0E
#define BSON_TYPE_CODE_W_SCOPE  0x0F // \x0F
#define BSON_TYPE_INT32         0x10 // \x10 int32 - 32-bit integer
#define BSON_TYPE_TIMESTAMP     0x11 // \x11
#define BSON_TYPE_INT64         0x12 // \x12 int64 - 64-bit integer.

/**
 * Helper struct to reference Bson elements
 */
struct BsonElem
{
public:
  uint8_t type;
  uint32_t dataLen;
  uint32_t elemLen;
  const uint8_t* data;
  const uint8_t* name;

  BsonElem& operator=(const BsonElem &a)
  {
    this->type = a.type;
    this->data = a.data;
    this->dataLen = a.dataLen;
    this->elemLen = a.elemLen;
    this->name = a.name;

    return *this;
  }
};


/**
 * This class implements a very simple memory allocator.
 * It reserves contiguous blocks of memory and does not support freeing of the memory.
 * It's useful when initialising the system and reserving DMA memory
 */
class BsonReader
{
private:
  void getField(const uint8_t* data, BsonElem& elem);

public:

  BsonReader();

  bool findField(const uint8_t* data, const char *fieldName, BsonElem& elem);
  uint32_t getArrayCount(const uint8_t* arrayOffset);
  uint32_t getInt32Array(int32_t* dst, const uint8_t* arrayOffset, uint32_t len);
  uint32_t getFloatArray(float32_t* dst, const uint8_t* arrayOffset, uint32_t len);
  uint32_t getIntPairArray(std::pair<uint8_t, uint8_t> *dst, const uint8_t *data, uint32_t maxCount);
};
