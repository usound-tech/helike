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
//  Description: VT-100 terminal compatible endpoint
//  Filename: Terminal.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "vt100.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "Cli.hpp"
#include <functional>

namespace Controller
{

// direction of history navigation
#define _HIST_UP   0
#define _HIST_DOWN 1

// esc seq internal codes
#define _ESC_BRACKET  1
#define _ESC_HOME     2
#define _ESC_END      3


class Terminal
{
private:
  char escape_seq;
  char escape;

  std::string basePrompt;

  std::string prompt;
  std::string cmdline;
  uint32_t cursor;
  int currentHistoryEntry;
  bool hasHistory;
  std::function<void(const char*)> print;
  std::function<void(char*, uint16_t*, uint32_t)> gets;
  StringFifo<32> historyFifo;
  cli::Cli cli;

private:
  void printLine(int pos, int cursor);

  void restoreLineFromHistory(int dir);
  void saveLineToHistory();
  void searchHistory(int dir);

  int insertText(char *text, int len);

  void resetCursor();
  void moveCursor(int offset);
  void backspace();
  void handleNewLine();
  int escapeProcess(char ch);

public:
  Terminal(bool hasHistory, std::function<void(const char*)> print, std::function<void(char*, uint16_t*, uint32_t)> gets);

  void init();
  void insertChar(uint8_t ch);
};

}
