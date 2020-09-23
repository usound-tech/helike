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
//  Description: Defines the global services monostate.
//  Filename: Services.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/Services.hpp"

Services::Services(SimpleMemAllocator *simpleMemAllocator, System::SystemController *systemController, System::SystemConfiguration *systemConfiguration,
    System::Oal *oal, System::AudioService *audioService, System::Filesystem *filesystem, System::SystemStatus *systemStatus) :
    simpleMemAllocator(simpleMemAllocator),
    systemController(systemController),
    systemConfiguration(systemConfiguration),
    oal(oal),
    audioService(audioService),
    filesystem(filesystem),
    systemStatus(systemStatus)
{

}


Services *GlobalServiceConsumer::globalServices = nullptr;

void GlobalServiceConsumer::setGlobalServices(Services *services)
{
  GlobalServiceConsumer::globalServices = services;
}
