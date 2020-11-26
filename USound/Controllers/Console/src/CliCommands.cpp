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
//  Filename: CliCommands.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include <Controllers/FilePlayer/pub/AudioPlayer.hpp>
#include "CliCommands.hpp"
#include "Controllers/Audio/pub/AudioService.hpp"
#include "Controllers/System/pub/SystemInterfaces.hpp"
#include "Interfaces/pub/SystemControl.hpp"
#include "vt100.hpp"
#include "gpio.h"
#include "Controllers/Filesystem/pub/Filesystem.hpp"
#include <cstring>

#define COUNT_OF(X) (sizeof(X) / sizeof(*X))

namespace cli
{

void CliCommands::getPeripheralStatus(const std::string &peripheralName, System::SystemPeripheral systemPeripheral, std::string &response)
{
  response.append(ANSI_YELLOW_NORMAL).append(peripheralName);

  System::Peripheral *peripheral = globalServices->getSystemController()->getPeripheral(systemPeripheral);
  if (!peripheral)
  {
    response.append(ANSI_MAGENTA_NORMAL).append("NOT SUPPORTED");
  }
  else
  {
    System::State state = peripheral->getState();
    if (state == System::State::UNINITIALISED || state == System::State::ERROR)
    {
      response.append(ANSI_RED_NORMAL);
    }
    else
    {
      response.append(ANSI_GREEN_NORMAL);
    }

    response.append(System::toString(state));
  }
}

void CliCommands::getSdcardStatus(std::string &response)
{
  response.append(ANSI_YELLOW_NORMAL).append("\nSDCARD FS:     ");

  if (globalServices->getFilesystem()->isMounted())
  {
    response.append(ANSI_GREEN_NORMAL).append("MOUNTED");
  }
  else
  {
    response.append(ANSI_RED_NORMAL).append("NOT MOUNTED");
  }

}

bool CliCommands::showBistStatus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  std::string response;

  getPeripheralStatus("\nDAC TWEETER:   ", System::SystemPeripheral::DAC_TWEETER, response);
  getPeripheralStatus("\nDAC WOOFER:    ", System::SystemPeripheral::DAC_WOOFER, response);
  getPeripheralStatus("\nWOOFER AMP R:  ", System::SystemPeripheral::WOOFER_AMP_R, response);
  getPeripheralStatus("\nWOOFER AMP L:  ", System::SystemPeripheral::WOOFER_AMP_L, response);
  getPeripheralStatus("\nMCLK PLL:      ", System::SystemPeripheral::MCLK_PLL, response);
  getSdcardStatus(response);
  response.append(ANSI_RESET).append("\n");

  print(response.c_str());

  return true;
}

/**
 * Runs a bist cycle on all peripherals. It may run periodicaly with a configurable delay.
 * Press any key to exit.
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::runBist(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  auto systemController = globalServices->getSystemController();
  std::string response;
  uint32_t delay = 0;

  std::string token = tokenizer.getNextToken();
  if (!token.empty())
  {
    try
    {
      delay = std::stoi(token);
    }
    catch (std::exception &e)
    {
      response.append(ANSI_RED_NORMAL).append("Invalid argument. Expected the delay between bist cycles (in sec)").append(ANSI_RESET).append("\n");
      print(response.c_str());
      return false;
    }
  }

  System::Peripheral *dacTweeter = systemController->getPeripheral(System::SystemPeripheral::DAC_TWEETER);
  System::Peripheral *dacWoofer = systemController->getPeripheral(System::SystemPeripheral::DAC_WOOFER);
  System::Peripheral *ampWooferR = systemController->getPeripheral(System::SystemPeripheral::WOOFER_AMP_R);
  System::Peripheral *ampWooferL = systemController->getPeripheral(System::SystemPeripheral::WOOFER_AMP_L);
  System::Peripheral *mclkPll = systemController->getPeripheral(System::SystemPeripheral::MCLK_PLL);

  uint16_t size;
  do
  {
    if (dacTweeter)
    {
      dacTweeter->doAction(System::Action::RESET);
    }

    if (dacWoofer)
    {
      dacWoofer->doAction(System::Action::RESET);
    }

    if (ampWooferR)
    {
      ampWooferR->doAction(System::Action::RESET);
    }

    if (ampWooferL)
    {
      ampWooferL->doAction(System::Action::RESET);
    }

    mclkPll->doAction(System::Action::RESET);

    globalServices->getFilesystem()->umount();
    globalServices->getFilesystem()->mount();

    showBistStatus(cmd, tokenizer, print, gets);

    if (delay > 0)
    {
      char ch;
      size = 1;

      // Exit loop on any keypress
      gets(&ch, &size, delay * 1000);
      if (size > 0)
      {
        delay = 0;
      }
    }
  }
  while (delay > 0);

  return true;
}

/**
 * Shows the button state in a loop.
 * The user can exit the loop by pressing any key
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::showButtonStatus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  auto systemController = globalServices->getSystemController();
  uint32_t delay = 100;
  char ch;
  uint16_t size;

  System::Gpio *joystickNorth = systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_NORTH);
  System::Gpio *joystickSouth = systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_SOUTH);
  System::Gpio *joystickWest = systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_WEST);
  System::Gpio *joystickEast = systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_EAST);
  System::Gpio *joystickCenter = systemController->getGpio(System::GpioInterface::GPIO_JOYSTICK_CENTER);

  System::Gpio *dipswitch1 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_1);
  System::Gpio *dipswitch2 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_2);
  System::Gpio *dipswitch3 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_3);
  System::Gpio *dipswitch4 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_4);

  System::Gpio *dipswitchCfg1 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_CFG_1);
  System::Gpio *dipswitchCfg2 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_CFG_2);
  System::Gpio *dipswitchCfg3 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_CFG_3);
  System::Gpio *dipswitchCfg4 = systemController->getGpio(System::GpioInterface::GPIO_DIPSWITCH_CFG_4);

  std::string status;
  status.reserve(128);

  MX_GPIO_ControlInterrupts(0);

  do
  {
    status.clear();
    status.append("\033[120D\033[2K");

    status.append(ANSI_YELLOW_NORMAL).append("Joystick: ").append(ANSI_RESET);
    status.append(!joystickNorth->get() ? "N" : " ");
    status.append(!joystickSouth->get() ? "S" : " ");
    status.append(!joystickWest->get() ? "W" : " ");
    status.append(!joystickEast->get() ? "E" : " ");
    status.append(!joystickCenter->get() ? "C" : " ");

    status.append(ANSI_YELLOW_NORMAL).append(" Dipswitches: ").append(ANSI_RESET);
    status.append(!dipswitch1->get() ? "1" : " ");
    status.append(!dipswitch2->get() ? "2" : " ");
    status.append(!dipswitch3->get() ? "3" : " ");
    status.append(!dipswitch4->get() ? "4" : " ");

    status.append(ANSI_YELLOW_NORMAL).append(" Dipswitch cfg: ").append(ANSI_RESET);
    status.append(!dipswitchCfg1->get() ? "1" : " ");
    status.append(!dipswitchCfg2->get() ? "2" : " ");
    status.append(!dipswitchCfg3->get() ? "3" : " ");
    status.append(!dipswitchCfg4->get() ? "4" : " ");

    print(status.c_str());

    size = 1;
    gets(&ch, &size, delay);
    if (size > 0)
    {
      delay = 0;
    }
  }
  while (delay > 0);

  MX_GPIO_ControlInterrupts(1);
  print("\n");

  return true;
}

bool CliCommands::playAudioPattern(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
#if TONE_GEN_ENABLED == 1
  uint8_t spinner[] = "\\|/-";
  uint32_t spinnerIdx = 0;
  uint32_t delay = 100;
  char ch;
  uint16_t size;
  auto audioService = globalServices->getAudioService();

  audioService->stopPlay();
  auto prevSrc = audioService->selectAudioSource(System::SystemAudioSource::AUDIO_SRC_GENERATOR);
  audioService->startPlay();

  std::string status;
  do
  {
    status.clear();
    status.append("\033[120D\033[2K");
    status.push_back(spinner[spinnerIdx++]);
    if (spinnerIdx >= 4)
    {
      spinnerIdx = 0;
    }
    print(status.c_str());

    size = 1;
    gets(&ch, &size, delay);
    if (size > 0)
    {
      delay = 0;
    }
  }
  while (delay > 0);

  audioService->stopPlay();
  audioService->selectAudioSource(prevSrc);
#endif

  print("\n");
  return true;
}

/**
 * Reads data from the specied bus.
 * Command format: read <bus> <address> <reg> <# of regs>
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::readFromBus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  uint8_t data[32];
  System::Bus *bus;
  uint32_t reg;
  uint16_t count;
  uint32_t address;
  std::string busName = tokenizer.getNextToken();
  std::string addressName = tokenizer.getNextToken();
  std::string regNumber = tokenizer.getNextToken();
  std::string countNumber = tokenizer.getNextToken();
  std::string response;

  if (busName.empty() || addressName.empty() || regNumber.empty() || countNumber.empty())
  {
    response.append(ANSI_RED_NORMAL).append("Malformed command\n").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }

  if (busName == "i2cT")
  {
    bus = globalServices->getSystemController()->getBus(System::SystemBus::I2C_TWEETER);
  }
  else
  {
    bus = globalServices->getSystemController()->getBus(System::SystemBus::I2C_WOOFER);
  }

  try
  {
    reg = std::stoi(regNumber, 0, 0);
    address = std::stoi(addressName, 0, 0);
    count = std::stoi(countNumber, 0, 0);
  }
  catch (std::exception &e)
  {
    response.append(ANSI_RED_NORMAL).append("Invalid argument. Expected number").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }

  if (bus->read(address, reg, 1, data, &count, 100) != System::Status::STATUS_OK)
  {
    response.append(ANSI_RED_NORMAL).append("Failed to perform bus operation").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }

  char *tmp = new char[64];
  for (uint16_t i = 0; i < count; i++)
  {
    sprintf(tmp, ANSI_YELLOW_NORMAL "0x%lx : " ANSI_RESET "0x%lx\n", (reg + i), (uint32_t) data[i]);
    print(tmp);
  }

  delete[] tmp;
  print("\n");

  return true;
}

/**
 * Writes to a system bus.
 * Command format: write <bus> <address> <reg> <val1> <val2> ...
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::writeToBus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  uint8_t data[16];
  System::Bus *bus;
  uint16_t reg;
  uint32_t address;
  std::string busName = tokenizer.getNextToken();
  std::string addressName = tokenizer.getNextToken();
  std::string regNumber = tokenizer.getNextToken();
  std::string response;

  if (busName.empty() || addressName.empty() || regNumber.empty())
  {
    response.append(ANSI_RED_NORMAL).append("Malformed command\n").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }

  if (busName == "i2cT")
  {
    bus = globalServices->getSystemController()->getBus(System::SystemBus::I2C_TWEETER);
  }
  else
  {
    bus = globalServices->getSystemController()->getBus(System::SystemBus::I2C_WOOFER);
  }

  try
  {
    reg = std::stoi(regNumber, 0, 0);
    address = std::stoi(addressName, 0, 0);
  }
  catch (std::exception &e)
  {
    response.append(ANSI_RED_NORMAL).append("Invalid argument. Expected number").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }

  uint16_t i;
  for (i = 0; i < 16; i++)
  {
    std::string regVal = tokenizer.getNextToken();
    if (regVal.empty())
    {
      break;
    }

    data[i] = std::stoi(regVal, 0, 0);
  }

  if (bus->write(address, reg, 1, data, i, 100) != System::Status::STATUS_OK)
  {
    response.append(ANSI_RED_NORMAL).append("Failed to perform bus operation").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }

  print("\n");
  return true;
}

/**
 * Lists all sdcard files.
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::listFiles(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  auto fs = globalServices->getFilesystem();
  std::string response;

  getSdcardStatus(response);
  response.append(ANSI_RESET).append("\n");
  print(response.c_str());

  auto handle = fs->startList(nullptr);
  do
  {
    const char *filename = fs->getNextListFileName(handle);
    if (filename)
    {
      response.clear();
      response.append(filename).append("\n");
      print(response.c_str());
    }
    else
    {
      fs = nullptr;
    }
  }
  while (fs);

  return true;
}

/**
 * Shows live audio player status
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::audioPlayerStatus(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  auto mp3Player = (Controller::AudioPlayer*) globalServices->getSystemController()->getAudioSource(System::SystemAudioSource::AUDIO_SRC_FILE);
  std::string response;
  uint32_t delay = 1000;
  uint16_t size;
  char ch;

  response.reserve(100);
  do
  {
    response.clear();
    response.append("\033[120D\033[2K");

    switch (mp3Player->getAudioState())
    {
      case Controller::AudioPlayerState::AP_UNCONFIGURED:
        response.append(ANSI_RED_NORMAL).append("UNCONFIGURED ");
        break;

      case Controller::AudioPlayerState::AP_IDLE:
        response.append(ANSI_BLUE_NORMAL).append("IDLE ");
        break;

      case Controller::AudioPlayerState::AP_PLAYING:
        {
        uint32_t trackTime;
        uint32_t playbackTime;
        char tmp[16];

        mp3Player->getPlaybackInfo(trackTime, playbackTime);
        uint32_t seconds = playbackTime % 60;

        response.append(ANSI_GREEN_NORMAL).append("PLAYING ").append(ANSI_YELLOW_NORMAL).append(mp3Player->getFilename());
        response.append(ANSI_RESET);

        sprintf(tmp, "[ %02lu:%02lu", playbackTime / 60, seconds);
        response.append(tmp);

        if (trackTime)
        {
          seconds = trackTime % 60;
          response.append(ANSI_RESET);

          sprintf(tmp, " of %02lu:%02lu", trackTime / 60, seconds);
          response.append(tmp);
        }

        response.append(" ]");
      }
        break;

      default:
        response.append(ANSI_RED_NORMAL).append("ERROR ");
        break;
    }

    print(response.c_str());
    size = 1;
    gets(&ch, &size, delay);
    if (size > 0)
    {
      delay = 0;
    }
  }
  while (delay > 0);

  print("\n");
  return true;
}

const CliCommand CliCommands::bistCommands[] =
    {
        { "status", "Shows BIST status", CliCommands::showBistStatus, 0 },
        {
            "run_bist",
            "Triggers a BIST cycle.\n"
                "\t\trun_bist [x]\n"
                "\t\t[x]: Optional. Specifies delay (in seconds) between successive rounds of bist\n"
                "\t\tIf [x] == 0 or missing, the command will run only one bist cycle.\n"
                "\t\tWhen running in a loop, press any key to exit",
            CliCommands::runBist, 0 },
        { "buttons", "Reports the button & switch state. Press any key to exit", CliCommands::showButtonStatus, 0 }

#if TONE_GEN_ENABLED == 1
        ,{ "audio", "Plays an audio pattern. Press any key to exit", CliCommands::playAudioPattern, 0 }
#endif
    };

const CliCommand CliCommands::systemCommands[] =
    {
        {
            "read",
            "Reads data from a bus.\n"
                "\t\tread <bus> <dev address> <reg> <# of regs>\n"
                "\t\tSupported buses: i2cT, i2cW",
            CliCommands::readFromBus,
            0
        },
        { "write", "Writes data to a bus", CliCommands::writeToBus, 0 },
        { "ls", "Lists sdcard files", CliCommands::listFiles, 0 },
        { "player", "Shows status of the audio player", CliCommands::audioPlayerStatus, 0 },
    };

const CliCommand CliCommands::audioCommands[] =
    {
        {
            "drc",
            "Configures a DRC block.\n"
                "\t\tdrc <block> [<enable> <attack duration msec> <release duration msec> <threshold mdB> <ratio> <post-DRC gain mdB>]\n"
                "\t\tSupported blocks: leveler, limiter\n"
                "\t\tWhen no argument provided, it lists the current configuration values\n"
                "\t\tenable: 1 or 0, attack: 0 - 65535, release: 0 - 65535, threshold: -100000 to 100000\n"
                "\t\tratio 2:1 is 2000, 4:1 is 4000, 1:2 is 500, etc..., post-DRC gain: -100000 to 100000\n",
            CliCommands::drcConfiguration,
            0
        },
        {
            "eq",
            "Configures an EQ block.\n"
                "\t\teq [<block> <cmd>]\n"
                "\t\tSupported blocks: master, xover\n"
                "\t\tSupported commands: enable, disable. If cmd is not provided, it'll show the configuration\n",
            CliCommands::eqConfiguration,
            0
        },
    };

const CliCommand CliCommands::commands[] = {
    { "bist", "Enters BIST menu", nullptr, COUNT_OF(CliCommands::bistCommands), CliCommands::bistCommands },
    { "system", "Performs low level system operations", nullptr, COUNT_OF(CliCommands::systemCommands), CliCommands::systemCommands },
    { "audio", "Configures audio blocks", nullptr, COUNT_OF(CliCommands::audioCommands), CliCommands::audioCommands }
};

const CliCommand CliCommands::rootCommand = { "/", "", nullptr, COUNT_OF(CliCommands::commands), CliCommands::commands };


}
