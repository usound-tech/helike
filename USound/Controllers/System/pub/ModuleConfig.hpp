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
//  Description: Defines which modules are enabled during the build process.
//  Filename: ModuleConfig.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

//!< When set to 1, it enables the POS configuration
#define CONFIG_POS 0

//!< When set to 1, it enables loading mp3 files
#define MP3_READER_MODULE_ENABLED 0

//!< When set to 1, it enables the ALA filter
#define ALA_MODULE_ENABLED 1

//!< When set to 1, the tone generator is included as an audio source
#define TONE_GEN_ENABLED 0

//!< When set to 1, it enables the UART console
#define UART_CONSOLE_ENABLED 0

//!< When set to 1, it enables file creation/updates on the SD CARD
#define SDCARD_WRITE_ENABLED 1

//!< When set to 1, it converts the host gain to the DAC value using a logarithmic scale
#define LOGARITHMIC_GAIN_ENABLED 1

//!< When set to 1, it maps theft audio channel to right and vice-versa
#define SWAP_AUDIO_CHANNELS 1

// When set to true, the audio channel is inverted just before DAC for both Tweeter and Woofer
// (to compensate for accidental phase inversion in Hardware)
#define INVERT_LEFT_CHANNEL false
#define INVERT_RIGHT_CHANNEL true