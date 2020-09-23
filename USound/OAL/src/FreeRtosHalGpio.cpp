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
//  Description: Hardware abstraction layer - Uart FreeRtos port
//  Filename: FreeRtosHalUart.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "FreeRtosHal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/System/pub/SystemController.hpp"
#include "Interfaces/pub/SystemControl.hpp"

namespace System
{

/**
 * FreeRtos compatible Gpio wrapper constructor.
 * @param handle
 */
FreeRtosGpio::FreeRtosGpio(GpioConfiguration *gpioConfig) :
    gpioConfig(gpioConfig),
    eventHandler(nullptr)
{
}

/**
 * Initialises UART wrapper and allocates DMA channels.
 * It starts an endless circular DMA on the RX DMA
 */
void FreeRtosGpio::init()
{

}

/**
 * Sets the gpio to the requested value
 * @param value
 */
void FreeRtosGpio::set(uint32_t value)
{
  HAL_GPIO_WritePin(static_cast<GPIO_TypeDef*>(gpioConfig->port), (uint16_t) gpioConfig->pin, (GPIO_PinState) value);
}

/**
 * Returns the current value of the gpio
 * @return
 */
uint32_t FreeRtosGpio::get()
{
  return HAL_GPIO_ReadPin(static_cast<GPIO_TypeDef*>(gpioConfig->port), (uint16_t) gpioConfig->pin);
}

/**
 * Returns the associated exti event.
 * @return non zero value if there is an associated exti event, 0 otherwise
 */
uint16_t FreeRtosGpio::getExtEvent()
{
  return gpioConfig->inputEvent;
}

/**
 * Runs the associated event handler (if any)
 */
void FreeRtosGpio::runEvent()
{
  if (eventHandler)
  {
    eventHandler(get(), GpioEventType::GEVT_SINGLE);
  }
}

/**
 * Registers a callback for the interrupt-generating GPIOs
 * @param handler the pointer to the callback or nullptr to clear the callback
 */
void FreeRtosGpio::setEventHandler(std::function<void(uint32_t, GpioEventType)> handler)
{
  //TODO: Add an intermediate debouncer
  eventHandler = handler;
}

void FreeRtosGpio::triggerInputEvent(uint16_t eventId)
{
  Gpio *gpio = globalServices->getSystemController()->getGpioForEvent(eventId);
  if (gpio)
  {
    gpio->runEvent();
  }
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
  FreeRtosGpio::triggerInputEvent(pin);
}


void FreeRtosGpioLongPress::timeoutEventCb(void *arg)
{
  FreeRtosGpioLongPress *gpio = static_cast<FreeRtosGpioLongPress*>(arg);
  gpio->timeoutEvent();
}

/**
 * FreeRTOS compatible gpio wrapper with lonk keypress capabilities
 */
FreeRtosGpioLongPress::FreeRtosGpioLongPress(GpioConfiguration *gpioConfig) :
    FreeRtosGpio(gpioConfig)
{

}

/**
 * Initialises the gpio and allocates the debounce timers
 */
void FreeRtosGpioLongPress::init()
{

  state = get() ? GpioLPState::UP : GpioLPState::DOWN;
  timer = osTimerNew(FreeRtosGpioLongPress::timeoutEventCb, osTimerOnce, this, nullptr);
}

/**
 * Runs the associated event handler (if any)
 */
void FreeRtosGpioLongPress::runEvent()
{
  if (!eventHandler)
  {
    return;
  }

  auto level = get();

  if ((state == GpioLPState::UP) && !level)
  {
    state = GpioLPState::DOWN_DEBOUNCE;
    osTimerStart(timer, 200);
  }
  else if ((state == GpioLPState::DOWN) && level)
  {
    state = GpioLPState::UP_DEBOUNCE;
    osTimerStart(timer, 200);
  }
}

/**
 * This callback function runs when a soft timeout event occurs
 */
void FreeRtosGpioLongPress::timeoutEvent()
{
  uint32_t longPressDelay = 250;
  GpioEventType evType = GpioEventType::GEVT_SOFT;

  if ((state == GpioLPState::UP_DEBOUNCE) || (state == GpioLPState::DOWN_DEBOUNCE))
  {
    state = get() ? GpioLPState::UP : GpioLPState::DOWN;
    longPressDelay = 1000;
    evType = GpioEventType::GEVT_SINGLE;
  }

  if (state == GpioLPState::DOWN)
  {
    if (eventHandler)
    {
      eventHandler(get(), evType);
    }
    osTimerStart(timer, longPressDelay);
  }
}

}
