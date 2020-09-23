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
//  Description: Command line interface - command execution
//  Filename: Cli.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include <string>
#include <functional>

#define MAX_MENU_LEVELS 4

namespace cli
{

/**
 * Helper class to tokenize the command line arguments
 */
class StringTokenizer
{
private:
  std::string &srcString;
  std::string::size_type prev;
  std::string::size_type next;

public:
  StringTokenizer(std::string &src);

  std::string getNextToken();
};

/**
 * This struct wraps a cli menu entry
 */
struct CliCommand
{
public:
  std::string name;
  std::string doc;
  std::function<
      bool(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print, std::function<void(char*, uint16_t*, uint32_t)> gets)> func;
  size_t childCount;
  const CliCommand *children;
  mutable uint32_t privData;
  std::function<void(const CliCommand &cmd)> cleanUp;

  bool hasSubmenus() const
  {
    return (childCount > 0);
  }

  bool operator==(const CliCommand &a) const
  {
    return (this->name == a.name) && (this->childCount == a.childCount);
  }
};

/**
 * This class processes input text commands and traverses the cli command tree to find matches
 * and execute the code.
 */
class Cli
{
private:
  static const std::string exitCmd;
  static const std::string helpCmd;


  const CliCommand *menuStack[MAX_MENU_LEVELS];
  uint32_t stackIdx;
  std::string prompt;
  std::function<void(const char *text)> print;
  std::function<void(char*, uint16_t*, uint32_t)> gets;

  void stripWhite(std::string &cmd);
  void updatePrompt();
  void printHelp();
  void exitSubmenu();

public:
  Cli(const CliCommand &rootMenu, std::function<void(const char *text)> print, std::function<void(char*, uint16_t*, uint32_t)> gets);

  bool processCommand(std::string &commandLine);

  const std::string& getPrompt() const
  {
    return prompt;
  }
};

}
