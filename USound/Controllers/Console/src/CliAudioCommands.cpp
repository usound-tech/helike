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
//  Description: Command line audio commands
//  Filename: CliAudioCommands.cpp
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

/**
 * Configures the system DRCs
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::drcConfiguration(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  std::string drc = tokenizer.getNextToken();
  std::string enabled = tokenizer.getNextToken();
  std::string attack = tokenizer.getNextToken();
  std::string release = tokenizer.getNextToken();
  std::string threshold = tokenizer.getNextToken();
  std::string ratio = tokenizer.getNextToken();
  std::string postGain = tokenizer.getNextToken();
  std::string response;


  bool readStatus = false;
  int argCount = (int) enabled.empty() + (int) attack.empty() + (int) release.empty() + (int) threshold.empty() + (int) ratio.empty() + (int) postGain.empty();

  if (drc.empty() || ((argCount != 6) && argCount != 0))
  {
    response.append(ANSI_RED_NORMAL).append("Malformed command\n").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }
  else if (argCount == 6)
  {
    readStatus = true;
  }


  auto filterConfig = globalServices->getSystemConfiguration()->getFilterConfiguration();

  System::DrcConfiguration &drcConfig = (drc == "leveler") ? filterConfig->levelerDrcConfig : filterConfig->limiterDrcConfig;

  if (readStatus)
  {
    char *tmp = new char[64];
    sprintf(tmp, "Enabled: %s\n", drcConfig.enabled ? "Yes" : "No");
    print(tmp);

    sprintf(tmp, "Attack duration: %li\n", (uint32_t) (drcConfig.attackDuration * 1000));
    print(tmp);

    sprintf(tmp, "Release duration: %li\n", (uint32_t) (drcConfig.releaseDuration * 1000));
    print(tmp);

    sprintf(tmp, "Threshold: %li\n", (uint32_t) (drcConfig.compressionThresholdFullScaleDb * 1000));
    print(tmp);

    sprintf(tmp, "Ratio: %li\n", (uint32_t) (drcConfig.compressionRatio * 1000));
    print(tmp);

    sprintf(tmp, "Post-DRC gain: %li\n", (uint32_t) (drcConfig.postGain * 1000));
    print(tmp);

    delete[] tmp;
    return true;
  }

  try
  {
    drcConfig.enabled = std::stoi(enabled, 0, 0);
    drcConfig.attackDuration = std::stoi(attack, 0, 0) / 1000.0f;
    drcConfig.releaseDuration = std::stoi(release, 0, 0) / 1000.0f;
    drcConfig.compressionThresholdFullScaleDb = std::stoi(threshold, 0, 0) / 1000.0f;
    drcConfig.compressionRatio = std::stoi(ratio, 0, 0) / 1000.0f;
    drcConfig.postGain = std::stoi(postGain, 0, 0) / 1000.0f;
  }
  catch (std::exception &e)
  {
    response.append(ANSI_RED_NORMAL).append("Invalid argument. Expected number").append(ANSI_RESET).append("\n");
    print(response.c_str());
    return false;
  }

  globalServices->getAudioService()->reconfigureFilters();
  print("\n");
  return true;
}

/**
 * Helper method that prints the EQ stages coefficients
 */
void CliCommands::printBiquads(float32_t *coeffs, std::function<void(const char *text)> print)
{
  std::string response;

  for (int i = 0; i < 8; i++)
  {
    response.clear();
    for (int j = 0; j < 5; j++)
    {
      response.append(std::to_string(coeffs[i * 5 + j])).append(" ");
    }
    print(response.c_str());
  }
}

/**
 * Configures the system DRCs
 * @param cmd
 * @param tokenizer
 * @param print
 * @param gets
 * @return
 */
bool CliCommands::eqConfiguration(const CliCommand &cmd, StringTokenizer &tokenizer, std::function<void(const char *text)> print,
    std::function<void(char*, uint16_t*, uint32_t)> gets)
{
  std::string eq = tokenizer.getNextToken();
  std::string stage = tokenizer.getNextToken();
#if 0
  std::string b0 = tokenizer.getNextToken();
  std::string b1 = tokenizer.getNextToken();
  std::string b2 = tokenizer.getNextToken();
  std::string a1 = tokenizer.getNextToken();
  std::string a2 = tokenizer.getNextToken();
#endif
  std::string response;

  bool isMaster = false;
  bool readStatus = false;
  //int argCount = (int) stage.empty();

  if (eq.empty())
  {
    //response.append(ANSI_RED_NORMAL).append("Malformed command\n").append(ANSI_RESET).append("\n");
    //print(response.c_str());
    //return false;
    //}
    //else if (argCount == 1)
    //{
    readStatus = true;
  }

  auto filterConfig = globalServices->getSystemConfiguration()->getFilterConfiguration();
  if (eq == "master")
  {
    isMaster = true;
  }
  else
  {
    isMaster = false;
  }

#if 0
  float32_t *coeffs;

  if (eq == "master_left")
  {
    isMaster = true;
    coeffs = filterConfig->masterEqCoeffs[System::MasterEqCoeffcientType::LEFT];
  }
  else if (eq == "master_right")
  {
    isMaster = true;
    coeffs = filterConfig->masterEqCoeffs[System::MasterEqCoeffcientType::RIGHT];
  }
  else if (eq == "xover_tweeter_left")
  {
    coeffs = filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::LEFT_TWEETER];
  }
  else if (eq == "xover_tweeter_right")
  {
    coeffs = filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::RIGHT_TWEETER];
  }
  else if (eq == "xover_woofer_left")
  {
    coeffs = filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::LEFT_WOOFER];
  }
  else
  {
    coeffs = filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::RIGHT_WOOFER];
  }
#endif
  if (readStatus)
  {
    char *tmp = new char[64];

    sprintf(tmp, "Configuration: %s\n", filterConfig->name.c_str());
    print(tmp);

    sprintf(tmp, "MasterEq: %s\n", filterConfig->masterEqEnabled ? "Enabled" : "Disabled");
    print(tmp);

    sprintf(tmp, "XoverEq: %s\n\n", filterConfig->xoverEqEnabled ? "Enabled" : "Disabled");
    print(tmp);

    delete[] tmp;

    //printBiquads(coeffs, print);
    print("\n");
    return true;
  }

  //uint32_t eqStage = 0xFFFFFFFF;
  bool eqEnabled = true;
  if (stage == "enable")
  {
    eqEnabled = true;
  }
  else
  {
    eqEnabled = false;
  }
  /*
   else
   {
   eqStage = std::stoi(stage, 0, 0);
   }
   */

#if 0
  if (eqStage != 0xFFFFFFFF)
  {

    //NOT SUPPORTED FOR NOW
    try
    {
      int coeffOffset = eqStage * 5;
      coeffs[coeffOffset] = std::stof(b0, nullptr);
      /*coeffs[coeffOffset] = std::stof(b0, 0);
       coeffs[coeffOffset + 1] = std::stof(b1, 0);
       coeffs[coeffOffset + 2] = std::stof(b2, 0);
       coeffs[coeffOffset + 3] = std::stof(a1, 0);
       coeffs[coeffOffset + 4] = std::stof(a2, 0);
       */
    }
    catch (std::exception &e)
    {
      response.append(ANSI_RED_NORMAL).append("Invalid argument. Expected floating point number").append(ANSI_RESET).append("\n");
      print(response.c_str());
      return false;
    }
  }
  else
#endif
  {
    if (isMaster)
    {
      filterConfig->masterEqEnabled = eqEnabled;
    }
    else
    {
      filterConfig->xoverEqEnabled = eqEnabled;
    }
  }

  globalServices->getAudioService()->reconfigureFilters();
  print("\n");
  return true;
}
}
