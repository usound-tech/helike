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
//  Description: Sdcard filesystem
//  Filename: Sdcard.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Controllers/Filesystem/pub/Filesystem.hpp"
#include "fatfs.h"

namespace System
{

/**
 * This helper class contains the structs necessary to iterate a filesystem folder
 */
class DirectoryIterator
{
public:
  DIR dir;
  FILINFO info;

};

/**
 * This class is a wrapper to all the supported sdcard file operations
 */
class SdcardFile: public File
{
private:
  FIL fileInfo;

public:
  bool open(const char *name, bool reportFailure) override;
  bool createOrTruncate(const char *name, bool reportFailure) override;
  void close() override;
  uint32_t read(uint8_t *dst, uint32_t len) override;
  uint32_t write(const uint8_t *src, uint32_t len) override;
  bool seek(uint8_t offs) override;
  void truncate();
};


/**
 * This class is a wrapper to all the supported sdcard filesystem operations.
 */
class SdcardFs: public Filesystem
{
private:
  bool mounted;
  FilesystemStatus status;

  FATFS sdcardFatFs;
  char sdcardkPath[4] = "0";

public:
  SdcardFs();

  void init() override;
  bool mount() override;
  void umount() override;
  bool createFolder(const char* path);

  void* startList(void *handle) override;
  const char* getNextListFileName(void *handle) override;
  void closeList(void *handle) override;
  bool statFile(const char* path, uint32_t* size, uint8_t* attributes) override;

  /**
   * Returns true if the filesystem is mounted
   * @return
   */
  bool isMounted() override
  {
    return mounted;
  }

  /**
   * Returns the FS status
   * @return
   */
  FilesystemStatus getStatus() override
  {
    return status;
  }

  /**
   * Creates a new empty file instance
   * @return
   */
  File* getFile() override
  {
    return new SdcardFile();
  }
};


}


