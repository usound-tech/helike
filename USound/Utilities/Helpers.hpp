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
//  Description: Generic helper methods to make life easier
//  Filename: Helpers.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include <string>
#include <exception>
#include <memory>

/*
 template<typename ... Args>
 std::string stringFormat(const std::string &format, Args ... args)
 {
 size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
 if (size <= 0)
 {
 throw "Error during formatting.";
 }

 std::unique_ptr<char[]> buf(new char[size]);
 snprintf(buf.get(), size, format.c_str(), args ...);

 return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
 }
 */
