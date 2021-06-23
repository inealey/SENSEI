//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/internal/TransferInfo.h>

#include <svtkm/internal/ArrayPortalVirtual.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

bool TransferInfoArray::valid(svtkm::cont::DeviceAdapterId devId) const noexcept
{
  return this->DeviceId == devId;
}

void TransferInfoArray::updateHost(
  std::unique_ptr<svtkm::internal::PortalVirtualBase>&& host) noexcept
{
  this->Host = std::move(host);
}

void TransferInfoArray::updateDevice(svtkm::cont::DeviceAdapterId devId,
                                     std::unique_ptr<svtkm::internal::PortalVirtualBase>&& hostCopy,
                                     const svtkm::internal::PortalVirtualBase* device,
                                     const std::shared_ptr<void>& deviceState) noexcept
{
  this->HostCopyOfDevice = std::move(hostCopy);
  this->DeviceId = devId;
  this->Device = device;
  this->DeviceTransferState = deviceState;
}

void TransferInfoArray::releaseDevice()
{
  this->DeviceId = svtkm::cont::DeviceAdapterTagUndefined{};
  this->Device = nullptr; //The device transfer state own this pointer
  if (this->DeviceTransferState == nullptr)
  { //When the DeviceTransferState is a nullptr it means that
    //that the device and host share memory. In that case we need to free
    //the host copy
    this->HostCopyOfDevice.reset(nullptr);
  }
  else
  {
    //The DeviceTransferState holds ownership of HostCopyOfDevice so we only
    //need to delete DeviceTransferState, as it will do the rest
    this->DeviceTransferState = nullptr; //release the device transfer state
    this->HostCopyOfDevice.release();
  }
}

void TransferInfoArray::releaseAll()
{
  this->Host.release(); //we own this pointer so release it
  this->releaseDevice();
}
}
}
}
