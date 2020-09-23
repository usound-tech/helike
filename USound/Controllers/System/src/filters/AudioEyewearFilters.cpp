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
//  Description: Audio glasses filter configuration
//  Filename: AudioGlassesFilters.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../../pub/SystemConfiguration.hpp"

namespace System
{
#if CONFIG_POS == 1
FilterConfiguration audioEyewearConfiguration = {
    .name = "Audio eyewear",

    .masterEqEnabled = false,
    .xoverEqEnabled = true,

    .masterEqCoeffs = {
        // Left master Eq channel
        {},

        // Right master Eq channel
        {}
    },

    .xoverEqCoeffs = {
        // x-over woofer left channel
        {
            0.45023753f, -0.72282529f, 0.38440922f, 1.44565059f, -0.66929351f,
            0.83731024f, -1.4879628f, 0.80115696f, 1.48796280f, -0.6384672f,
            1.00667826f, -1.95977796f, 0.95449983f, 1.96008435f, -0.96087170f,
            0.94518069f, -0.94608650f, 0.75773001f, 0.94608650f, -0.70291070f,
            0.71360859f, -1.35553528f, 0.64538419f, 1.91545766f, -0.91891515f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f
        },

        // x-over tweeter left channel
        {
            0.42439162f, -0.70228970f, 0.29982975f, 1.78065106f, -0.80258274f,
            1.82705279f, -1.55452603f, 0.55552049f, 0.36967011f, -0.19771735f,
            0.96670789f, -0.88600709f, 0.80530629f, 0.88600709f, -0.77201418f,
            0.77917590f, -1.558351801f, 0.7791759f, 1.50865617f, -0.60804744f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f
        },

        // x-over woofer right channel
        {
            0.45023753f, -0.72282529f, 0.38440922f, 1.44565059f, -0.66929351f,
            0.83731024f, -1.4879628f, 0.80115696f, 1.48796280f, -0.6384672f,
            1.00667826f, -1.95977796f, 0.95449983f, 1.96008435f, -0.96087170f,
            0.94518069f, -0.94608650f, 0.75773001f, 0.94608650f, -0.70291070f,
            0.71360859f, -1.35553528f, 0.64538419f, 1.91545766f, -0.91891515f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f
        },

        // x-over tweeter right channel
        {
            0.42439162f, -0.70228970f, 0.29982975f, 1.78065106f, -0.80258274f,
            1.82705279f, -1.55452603f, 0.55552049f, 0.36967011f, -0.19771735f,
            0.96670789f, -0.88600709f, 0.80530629f, 0.88600709f, -0.77201418f,
            0.77917590f, -1.558351801f, 0.7791759f, 1.50865617f, -0.60804744f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f,
            1.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f
        }
    },

    .levelerDrcConfig = {
        .enabled = false
    },

    .limiterDrcConfig = {
        .enabled = false
    }
};
#endif
}
