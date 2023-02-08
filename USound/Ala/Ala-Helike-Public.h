#pragma once

#include <stdint.h>

void ApplyAla(float* signal, uint8_t* channelBuffer, uint8_t* globalBuffer, uint8_t* coefficientBuffer, int length);
void InitAlaChannelBuffer(uint8_t* channelBuffer);
void InitAlaCoefficients(uint8_t* coefficientBuffer);
