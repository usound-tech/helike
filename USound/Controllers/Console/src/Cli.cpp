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
//  Description: Command line interface
//  Filename: Cli.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Utilities/Logger/pub/Logger.h"
#include "Cli.hpp"
#include "vt100.hpp"

#if UART_CONSOLE_ENABLED == 1

namespace cli
{

//!< Initialise static string member exit
const std::string Cli::exitCmd = "exit";

//!< Initialise static string member help
const std::string Cli::helpCmd = "help";


/**
 * Cli constructor.
 *
 * @param rootMenu the top level pointer
 */
Cli::Cli(const CliCommand &rootMenu, std::function<void(const char *text)> print, std::function<void(char*, uint16_t*, uint32_t)> gets) :
    stackIdx(0),
    print(print),
    gets(gets)
{
  prompt.reserve(32);
  menuStack[0] = &rootMenu;
  prompt = "/";
}


/**
 * It processes a full command line.
 * The operation is intrusive and destroys the input buffer by adding extra \0 characters while tokenizing it.
 *
 * @param commandLine the full command line (cmd and input arguments)
 */
bool Cli::processCommand(std::string &commandLine)
{
  const CliCommand *cliCommandMenu = menuStack[stackIdx];
  const CliCommand *cliCommandPtr = nullptr;

  StringTokenizer tokenizer = StringTokenizer(commandLine);
  std::string cmd = tokenizer.getNextToken();

  if (!helpCmd.compare(cmd))
  {
    printHelp();
    return true;
  }

  if (!exitCmd.compare(cmd))
  {
    exitSubmenu();
    return true;
  }

  for (uint32_t i = 0; i < cliCommandMenu->childCount; i++)
  {
    if (!cliCommandMenu->children[i].name.compare(cmd))
    {
      cliCommandPtr = &cliCommandMenu->children[i];
    }
  }

  if (!cliCommandPtr)
  {
    print("Command not found\n");
    return false;
  }

  const CliCommand &cliCommand = *cliCommandPtr;

  if (cliCommand.func)
  {
    bool result = cliCommand.func(cliCommand, tokenizer, print, gets);

    if (!result)
    {
      return false;
    }
  }

  if (cliCommand.hasSubmenus())
  {
    if ((stackIdx + 1) >= MAX_MENU_LEVELS)
    {
      print("Max nested menu level reached\n");
      return false;
    }

    stackIdx++;
    menuStack[stackIdx] = cliCommandPtr;

    updatePrompt();
  }

  return true;
}

/**
 * Returns to the parent level (if the current level is not the root level)
 */
void Cli::exitSubmenu()
{
  const CliCommand &cliCommandMenu = *menuStack[stackIdx];

  if (stackIdx > 0)
  {
    if (cliCommandMenu.cleanUp)
    {
      cliCommandMenu.cleanUp(cliCommandMenu);
    }

    stackIdx--;
    updatePrompt();
  }
  else
  {
    print("Already at top level\n");
  }
}

void Cli::printHelp()
{
  std::string helpText;
  helpText.reserve(128);
  const CliCommand *cliCommandMenu = menuStack[stackIdx];

  for (uint32_t i = 0; i < cliCommandMenu->childCount; i++)
  {
    helpText.append("    ").append(cliCommandMenu->children[i].name).append(" : ");
    print(helpText.c_str());
    helpText.clear();

    print(cliCommandMenu->children[i].doc.c_str());
    print("\n");
  }

  helpText.append("    help : Shows this help message\n");
  helpText.append("    exit : Returns to the parent level\n");

  print(helpText.c_str());
}

/**
 * Removes whitespace characters from the beginning and end of a string (in-place).
 * This function modifies the original string, so be careful.
 *
 * @param cmd the input string to be processed
 * @return the pointer to the stripped string
 */
void Cli::stripWhite(std::string &cmd)
{
  const char *ws = " \t\n\r\f\v";

  cmd.erase(0, cmd.find_first_not_of(ws)).erase(cmd.find_last_not_of(ws) + 1);
}


/**
 * Updates the prompt to show the nested levels
 */
void Cli::updatePrompt()
{
  prompt.clear();
  prompt = "/";

  for (uint32_t i = 1; i <= stackIdx; i++)
  {
    prompt.append(menuStack[i]->name).append("/");
  }
}


StringTokenizer::StringTokenizer(std::string &src) :
    srcString(src),
    prev(0),
    next(0)
{

}

std::string StringTokenizer::getNextToken()
{
  static const std::string delims(" \t,.;");

  std::string token;

  prev = srcString.find_first_not_of(delims, prev);
  if (prev != std::string::npos)
  {
    next = srcString.find_first_of(delims, prev);
    token = srcString.substr(prev, next - prev);
    prev = next;
  }

  return token;
}

}
#endif
