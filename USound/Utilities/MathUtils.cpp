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
//  Description: Useful math operations
//  Filename: MathUtils.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================


#include "MathUtils.hpp"
#include "cmath"

/**
 *  Fast approximation to the log2() function. It uses a two step
 *  process. First, it decomposes the floating-point number into
 *  a fractional component F and an exponent E. The fraction component
 *  is used in a polynomial approximation and then the exponent added
 *  to the result. A 3rd order polynomial is used and the result
 *  when computing db20() is accurate to 7.984884e-003 dB.
 *
 *  https://community.arm.com/tools/f/discussions/4292/cmsis-dsp-new-functionality-proposal/22621#22621
 */
extern "C"
float32_t log2fApprox(float32_t X)
{
  float Y;
  float F;
  int E;

  // This is the approximation to log2()
  F = frexpf(fabsf(X), &E);
  Y = 1.23149591368684f;
  Y *= F;
  Y += -4.11852516267426f;
  Y *= F;
  Y += 6.02197014179219f;
  Y *= F;
  Y += -3.13396450166353f;
  Y += E;

  return (Y);
}

/**
 * Accelerate the powf(10.0,x) function
 */
extern "C"
float32_t pow10f(float32_t x)
{
  //return powf(10.0f,x)   //standard, but slower
  return expf(2.302585092994f * x);  //faster:  exp(log(10.0f)*x)
}

/**
 * Accelerate the log10f(x) function
 */
extern "C"
float32_t log10fApprox(float32_t x)
{
  //return log10f(x);   //standard, but slower
  return log2fApprox(x) * 0.3010299956639812f; //faster:  log2(x)/log2(10)
}
