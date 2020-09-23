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
//  Description: Filesystem generic interface
//  Filename: Filesystem.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Controllers/Service/pub/Services.hpp"


namespace System
{

enum FilesystemStatus
{
  FS_UNINITIALISED,
  FS_ENABLED,
  FS_BUSY,
  FS_ERROR
};

enum FilesystemType
{
  FILESYSTEM_SDCARD
};

/**
 * This abstract class exposes all the supported file operations
 */
class File: public GlobalServiceConsumer
{
public:
  virtual bool open(const char *name, bool reportFailure) = 0;
  virtual void close() = 0;
  virtual uint32_t read(uint8_t *dst, uint32_t len) = 0;
  virtual bool seek(uint8_t offs) = 0;

  virtual ~File()
  {
  }
};

/**
 * This abstract class exposes all the supported filesystem operations
 */
class Filesystem: public GlobalServiceConsumer
{
public:
  virtual void init() = 0;
  virtual bool mount() = 0;
  virtual void umount() = 0;
  virtual bool isMounted() = 0;

  virtual void* startList(void *handle) = 0;
  const virtual char* getNextListFileName(void *handle) = 0;
  virtual void closeList(void *handle) = 0;
  virtual bool statFile(const char* path, uint32_t* size, uint8_t* attributes) = 0;

  virtual FilesystemStatus getStatus() = 0;
  virtual File* getFile() = 0;

  virtual ~Filesystem()
  {
  }
};


}
