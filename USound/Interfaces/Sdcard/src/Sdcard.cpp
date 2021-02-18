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
//  Filename: Sdcard.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "fatfs.h"
#include "../pub/Sdcard.hpp"
#include "Controllers/System/pub/SystemStatus.hpp"
#include "OAL/pub/Oal.hpp"
#include <memory>

namespace System
{

SdcardFs::SdcardFs() :
    mounted(false),
    status(FilesystemStatus::FS_UNINITIALISED)
{

}

void SdcardFs::init()
{
  memset(&sdcardFatFs, 0, sizeof(FATFS));
}

/**
 * Creates a new folder
 * @param path
 * @return true if the folder is created
 */
bool SdcardFs::createFolder(const char* path)
{
  return f_mkdir(path) == FR_OK;
}

/**
 * Mounts the fatfs
 * @return
 */
bool SdcardFs::mount()
{
  if (mounted)
  {
    return true;
  }

  bool isMountSuccessful = false;
  for (int retries = 0; retries < 5; retries++)
  {
    if (f_mount(&sdcardFatFs, (const TCHAR*) sdcardkPath, 1) == FR_OK)
    {
      isMountSuccessful = true;
      break;
    }
    else
    {
      globalServices->getOal()->delay(500);
    }
  }

  if (!isMountSuccessful)
  {
    globalServices->getSystemStatus()->raiseError(ErrorStatus::ERS_SDCARD_FAILED);
    status = FilesystemStatus::FS_ERROR;
    return false;
  }

  mounted = true;
  status = FilesystemStatus::FS_ENABLED;
  return true;
}

/**
 * Umounts the FATFS
 */
void SdcardFs::umount()
{
  if (!mounted)
  {
    return;
  }

  f_mount(nullptr, (const TCHAR*) sdcardkPath, 0);
  mounted = false;
}

/**
 * Starts listing the files of a folder. If a previously opened iterator is provided, it'll rewind to the beginning of the entries.
 * @param handle a previously opened directory handle, or nullptr
 * @return
 */
void* SdcardFs::startList(void *handle)
{
  if (!mounted)
  {
    return nullptr;
  }

  DirectoryIterator *dirIterator = (DirectoryIterator*) handle;
  const char *path = "/";
  FRESULT res;

  if (!dirIterator)
  {
    dirIterator = new DirectoryIterator;

    res = f_opendir(&dirIterator->dir, path);
    if (res != FR_OK)
    {
      globalServices->getSystemStatus()->raiseError(ErrorStatus::ERS_SDCARD_FAILED);

      delete dirIterator;
      dirIterator = nullptr;
    }
  }
  else
  {
    //Rewind list
    if (f_readdir(&dirIterator->dir, nullptr) != FR_OK)
    {
      delete dirIterator;
      dirIterator = nullptr;
    }
  }

  return dirIterator;
}

/**
 * Checks for the presence of the file and returns the file info
 *
 * @param path the path of the file to be checked
 * @param size the file size
 * @param attributes the file attributes
 * @return
 */
bool SdcardFs::statFile(const char* path, uint32_t* size, uint8_t* attributes)
{
  if (!mounted)
  {
    return false;
  }

  FILINFO* info = new FILINFO;
  FRESULT res = f_stat(path, info);
  if (res == FR_OK)
  {
    *size = (uint32_t) info->fsize;
    *attributes = (uint8_t) info->fattrib;
  }

  delete info;
  return (res == FR_OK);
}

/**
 * Reads the next directory item
 * @return
 */
const char* SdcardFs::getNextListFileName(void *handle)
{
  DirectoryIterator *dirIterator = (DirectoryIterator*) handle;
  if (!dirIterator)
  {
    return nullptr;
  }

  bool isFile;

  do
  {
    FRESULT res = f_readdir(&dirIterator->dir, &dirIterator->info); /* Read a directory item */

    if (res != FR_OK)
    {
      globalServices->getSystemStatus()->raiseError(ErrorStatus::ERS_SDCARD_FAILED);
      return nullptr;
    }
    else if (dirIterator->info.fname[0] == 0)
    {
      return nullptr;
    }
    else
    {
      isFile = !(dirIterator->info.fattrib & AM_DIR);
    }
  }
  while (!isFile);

  return dirIterator->info.fname;
}

void SdcardFs::closeList(void *handle)
{
  DirectoryIterator *dirIterator = (DirectoryIterator*) handle;
  if (!dirIterator)
  {
    return;
  }

  f_closedir(&dirIterator->dir);
  delete dirIterator;
}

/**
 * Opens a file from the sdcard and returns true when successful
 * @param name
 * @param flags
 * @return
 */
bool SdcardFile::open(const char *name, bool reportFailure)
{
  FRESULT status = f_open(&fileInfo, name, FA_READ);
  if ((status != FR_OK) && reportFailure)
  {
    globalServices->getSystemStatus()->raiseError(ErrorStatus::ERS_SDCARD_FAILED);
  }

  return status == FR_OK;
}

/**
 * Opens a file from the sdcard and returns true when successful
 * @param name
 * @param flags
 * @return
 */
bool SdcardFile::createOrTruncate(const char *name, bool reportFailure)
{
  FRESULT status = f_open(&fileInfo, name, FA_WRITE|FA_CREATE_ALWAYS);
  if ((status != FR_OK) && reportFailure)
  {
    globalServices->getSystemStatus()->raiseError(ErrorStatus::ERS_SDCARD_FAILED);
  }

  return status == FR_OK;
}

/**
 * Closes a previously opened file
 */
void SdcardFile::close()
{
  f_close(&fileInfo);
}

/**
 * Truncates a previously opened file
 */
void SdcardFile::truncate()
{
  f_truncate(&fileInfo);
}

/**
 * Reads a requested amount of bytes from an sdcard file
 * @param dst
 * @param len
 * @return
 */
uint32_t SdcardFile::read(uint8_t *dst, uint32_t len)
{
  uint32_t bytesRead;

  if (f_read(&fileInfo, (void*) dst, (UINT) len, (UINT*) &bytesRead) != FR_OK)
  {
    globalServices->getSystemStatus()->raiseError(ErrorStatus::ERS_SDCARD_FAILED);
    bytesRead = 0;
  }

  return bytesRead;
}

/**
 * Reads a requested amount of bytes from an sdcard file
 * @param dst
 * @param len
 * @return
 */
uint32_t SdcardFile::write(const uint8_t *src, uint32_t len)
{
#if SDCARD_WRITE_ENABLED == 1
  uint32_t bytesWritten;

  if (f_write(&fileInfo, (const void*) src, (UINT) len, (UINT*) &bytesWritten) != FR_OK)
  {
    globalServices->getSystemStatus()->raiseError(ErrorStatus::ERS_SDCARD_FAILED);
    bytesWritten = 0;
  }

  return bytesWritten;
#else
  return 0;
#endif

}

/**
 * Moves the file reading pointer to the requested offset (from the beginning of the file)
 * @param offs
 * @return
 */
bool SdcardFile::seek(uint8_t offs)
{
  return f_lseek(&fileInfo, offs) == FR_OK;
}

}

