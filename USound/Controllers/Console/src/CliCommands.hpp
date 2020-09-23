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
//  Description: Command line interface commands
//  Filename: CliCommands.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Cli.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemController.hpp"
#include "Interfaces/pub/SystemControl.hpp"

namespace cli
{

/**
 * This class contains all the cli commands and menu tree
 */
class CliCommands: public GlobalServiceConsumer
{
private:
  static const CliCommand bistCommands[];
  static const CliCommand systemCommands[];
  static const CliCommand audioCommands[];
  static const CliCommand commands[];
  static const CliCommand rootCommand;

private:
  static void getPeripheralStatus(const std::string &peripheralName, System::SystemPeripheral systemPeripheral, std::string &response);
  static void getSdcardStatus(std::string &response);

  static bool showBistStatus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);
  static bool runBist(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);
  static bool showButtonStatus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);
  static bool playAudioPattern(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);

  static bool readFromBus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);
  static bool writeToBus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);

  static bool listFiles(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);
  static bool audioPlayerStatus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);

  static bool drcConfiguration(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);
  static bool eqConfiguration(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
      std::function<void(char*, uint16_t*, uint32_t)> gets);

  static void printBiquads(float32_t *coeffs, std::function<void(const char *text)> print);

public:
  static const CliCommand& getRootCommand()
  {
    return rootCommand;
  }
};


}
