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
//  Description: The console interface controller
//  Filename: ConsoleController.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/System/pub/SystemController.hpp"
#include "Controllers/Console/pub/Console.hpp"
#include "OAL/pub/Oal.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "Terminal.hpp"
#include "CliCommands.hpp"

namespace Controller
{
#define COMMAND_LINE (128)
#define ENDL "\n"

#if UART_CONSOLE_ENABLED == 1

Terminal::Terminal(bool hasHistory, std::function<void(const char*)> print, std::function<void(char*, uint16_t*, uint32_t)> gets) :
    escape_seq(0),
    escape(0),
    cursor(0),
    currentHistoryEntry(0),
    hasHistory(hasHistory),
    print(print),
    gets(gets),
    historyFifo(nullptr, 0),
    cli(cli::CliCommands::getRootCommand(), print, gets)
{
}

/**
 * Initialises the terminal
 */
void Terminal::init()
{
  basePrompt = "> ";
  prompt = cli.getPrompt() + basePrompt;
  cmdline.reserve(COMMAND_LINE);

  if (hasHistory)
  {
    historyFifo.reset(new uint8_t[128], 128);
  }
}

/**
 * Pushes the current line to history
 */
void Terminal::saveLineToHistory()
{
  historyFifo.pushBufferChunk((const uint8_t*) cmdline.c_str(), cmdline.size());
  currentHistoryEntry = 0;
}

/**
 * Copies a history entry to current line
 * @param line
 * @param dir
 * @return
 */
void Terminal::restoreLineFromHistory(int dir)
{
  int chunkCount = (int) historyFifo.getChunkCount();

  cmdline.clear();

  if (dir == _HIST_UP)
  {
    if (currentHistoryEntry < chunkCount)
    {
      currentHistoryEntry++;
    }

    if (!historyFifo.getChunkToString(currentHistoryEntry - 1, cmdline))
    {
      return;
    }
  }
  else
  {
    if (currentHistoryEntry > 0)
    {
      currentHistoryEntry--;

      if (currentHistoryEntry > 0)
      {
        if (!historyFifo.getChunkToString(currentHistoryEntry - 1, cmdline))
        {
          return;
        }
      }
    }
  }
}

/**
 * Sends the VT100 escape sequences to remote terminal to move the cursor
 * @param offset
 */
void Terminal::moveCursor(int offset)
{
  std::string str;

  if (offset > 0)
  {
    str.append("\033[").append(std::to_string(offset)).append("C");
  }
  else if (offset < 0)
  {
    str.append("\033[").append(std::to_string(-offset)).append("D");
  }

  print(str.c_str());
}

/**
 * Moves the remote cursor to the beginning of the line
 */
void Terminal::resetCursor()
{
  //char str[16];
  std::string str;
  str.append("\033[").append(std::to_string(COMMAND_LINE + prompt.size() + 2)).append("D");
  str.append("\033[").append(std::to_string(prompt.size())).append("C");
  print(str.c_str());
}

/**
 * Prints the line from the requested position to the end and also moves the remote cursor
 * @param pos
 * @param cursor
 */
void Terminal::printLine(int pos, int cursor)
{
  print((const char*) "\033[K");    // delete all from cursor to end

  char nch[] = { 0, 0 };
  int i;

  int len = (int) cmdline.size();
  for (i = pos; i < len; i++)
  {
    nch[0] = cmdline[i];

    if (nch[0] == '\0')
    {
      nch[0] = ' ';
    }
    print(nch);
  }

  resetCursor();
  moveCursor(cursor);
}


/**
 * Brings the previous/next history entry and displays the result
 * @param dir
 */
void Terminal::searchHistory(int dir)
{
  if (!hasHistory)
  {
    return;
  }

  restoreLineFromHistory(dir);
  cursor = cmdline.size();

  resetCursor();
  printLine(0, cursor);
}

/**
 * Processes VT100 escape sequences, if an escape character was received before
 * @param ch
 * @return
 */
int Terminal::escapeProcess(char ch)
{
  if (ch == '[')
  {
    escape_seq = _ESC_BRACKET;
    return 0;
  }
  else if (escape_seq == _ESC_BRACKET)
  {
    if (ch == 'A')
    {
      searchHistory(_HIST_UP);
      return 1;
    }
    else if (ch == 'B')
    {
      searchHistory(_HIST_DOWN);
      return 1;
    }
    else if (ch == 'C')
    {
      if (cursor < cmdline.size())
      {
        moveCursor(1);
        cursor++;
      }
      return 1;
    }
    else if (ch == 'D')
    {
      if (cursor > 0)
      {
        moveCursor(-1);
        cursor--;
      }
      return 1;
    }
    else if (ch == '7')
    {
      escape_seq = _ESC_HOME;
      return 0;
    }
    else if (ch == '8')
    {
      escape_seq = _ESC_END;
      return 0;
    }
  }
  else if (ch == '~')
  {
    if (escape_seq == _ESC_HOME)
    {
      resetCursor();
      cursor = 0;
      return 1;
    }
    else if (escape_seq == _ESC_END)
    {
      moveCursor(cmdline.size() - cursor);
      cursor = cmdline.size();
      return 1;
    }
  }

  /* unknown escape sequence, stop */
  return 1;
}

/**
 * Insert a buffer to command line, without processing for escape characters
 * @param text
 * @param len
 * @return
 */
int Terminal::insertText(char *text, int len)
{
  if ((cmdline.size() + len) < COMMAND_LINE)
  {
    cmdline.insert(cursor, text, len);
    cursor += len;
    return true;
  }

  return false;
}

/**
 * Erases once character (to the left) from the current cursor position
 */
void Terminal::backspace()
{
  if (cursor > 0)
  {
    print("\033[D \033[D");
    cmdline.erase(cursor - 1, 1);
    cursor--;
  }
}

/**
 * Handles a LF key press (full line)
 */
void Terminal::handleNewLine()
{
  print(ENDL);

  if (!cmdline.empty())
  {
    saveLineToHistory();
  }

  cli.processCommand(cmdline);

  prompt = cli.getPrompt() + basePrompt;
  print(prompt.c_str());

  cursor = 0;
  cmdline.clear();
}

/**
 * Process a single character from UART
 * @param ch
 */
void Terminal::insertChar(uint8_t ch)
{
  if (escape)
  {
    if (escapeProcess(ch))
      escape = 0;
  }
  else
  {
    switch (ch)
    {
      case KEY_CR:
        break;

      case KEY_LF:
        handleNewLine();
        break;

      case KEY_ESC:
        escape = 1;
        break;

      case KEY_NAK: // ^U
        while (cursor > 0)
        {
          backspace();
        }

        printLine(0, cursor);
        break;

      case KEY_VT:  // ^K - Erase from cursor till end of line
        print("\033[K");
        cmdline.erase(cursor);
        break;

      case KEY_ENQ: // ^E - Move cursor to end of line
        moveCursor(cmdline.size() - cursor);
        cursor = cmdline.size();
        break;

      case KEY_SOH: // ^A - Move cursor to start of line
        resetCursor();
        cursor = 0;
        break;

      case KEY_ACK: // ^F - Move cursor to the right
        if (cursor < cmdline.size())
        {
          moveCursor(1);
          cursor++;
        }
        break;

      case KEY_STX: // ^B - Move cursor to the left
        if (cursor)
        {
          moveCursor(-1);
          cursor--;
        }
        break;

      case KEY_DLE: //^P - Move to previous history entry
        searchHistory(_HIST_UP);
        break;

      case KEY_SO: //^N - Move to next history entry
        searchHistory(_HIST_DOWN);
        break;

      case KEY_DEL: // Backspace
      case KEY_BS: // ^U
        backspace();
        printLine(cursor, cursor);
        break;

      case KEY_DC2: // ^R - Reset line
        print(ENDL);
        print(prompt.c_str());

        resetCursor();
        printLine(0, cursor);
        break;

#ifdef _USE_CTLR_C
        //TODO: Exist menu nesting
      case KEY_ETX:
      if (pThis->sigint != NULL)
        pThis->sigint();
      break;
#endif

      default:
        if (((ch == ' ') && cmdline.empty()) || IS_CONTROL_CHAR(ch))
        {
          break;
        }

        if (insertText((char*) &ch, 1))
        {
          printLine(cursor - 1, cursor);
        }
        break;
    }
  }
}
#endif

}
