//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <cstdlib>
#include <mutex>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/cuda/ErrorCuda.h>
#include <svtkm/cont/cuda/internal/CudaAllocator.h>
#define NO_SVTKM_MANAGED_MEMORY "NO_SVTKM_MANAGED_MEMORY"

#include <mutex>
#include <vector>

SVTKM_THIRDPARTY_PRE_INCLUDE
#include <cuda_runtime.h>
SVTKM_THIRDPARTY_POST_INCLUDE

// These static vars are in an anon namespace to work around MSVC linker issues.
namespace
{
#if CUDART_VERSION >= 8000
// Has CudaAllocator::Initialize been called by any thread?
static std::once_flag IsInitialized;
#endif

// Holds how SVTK-m currently allocates memory.
// When SVTK-m is initialized we set this based on the hardware support ( HardwareSupportsManagedMemory ).
// The user can explicitly disable managed memory through an enviornment variable
// or by calling a function on the CudaAllocator.
// Likewise managed memory can be re-enabled by calling a function on CudaAllocator
// if and only if the underlying hardware supports pageable managed memory
static bool ManagedMemoryEnabled = false;

// True if concurrent pagable managed memory is supported by the machines hardware.
static bool HardwareSupportsManagedMemory = false;

// Avoid overhead of cudaMemAdvise and cudaMemPrefetchAsync for small buffers.
// This value should be > 0 or else these functions will error out.
static std::size_t Threshold = 1 << 20;
}

namespace svtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

bool CudaAllocator::UsingManagedMemory()
{
  CudaAllocator::Initialize();
  return ManagedMemoryEnabled;
}

void CudaAllocator::ForceManagedMemoryOff()
{
  if (HardwareSupportsManagedMemory)
  {
    ManagedMemoryEnabled = false;
    SVTKM_LOG_F(svtkm::cont::LogLevel::Info, "CudaAllocator disabling managed memory");
  }
  else
  {
    SVTKM_LOG_F(
      svtkm::cont::LogLevel::Warn,
      "CudaAllocator trying to disable managed memory on hardware that doesn't support it");
  }
}

void CudaAllocator::ForceManagedMemoryOn()
{
  if (HardwareSupportsManagedMemory)
  {
    ManagedMemoryEnabled = true;
    SVTKM_LOG_F(svtkm::cont::LogLevel::Info, "CudaAllocator enabling managed memory");
  }
  else
  {
    SVTKM_LOG_F(svtkm::cont::LogLevel::Warn,
               "CudaAllocator trying to enable managed memory on hardware that doesn't support it");
  }
}

bool CudaAllocator::IsDevicePointer(const void* ptr)
{
  CudaAllocator::Initialize();
  if (!ptr)
  {
    return false;
  }

  cudaPointerAttributes attr;
  cudaError_t err = cudaPointerGetAttributes(&attr, ptr);
  // This function will return invalid value if the pointer is unknown to the
  // cuda runtime. Manually catch this value since it's not really an error.
  if (err == cudaErrorInvalidValue)
  {
    cudaGetLastError(); // Clear the error so we don't raise it later...
    return false;
  }
  SVTKM_CUDA_CALL(err /*= cudaPointerGetAttributes(&attr, ptr)*/);
  return attr.devicePointer == ptr;
}

bool CudaAllocator::IsManagedPointer(const void* ptr)
{
  if (!ptr || !ManagedMemoryEnabled)
  {
    return false;
  }

  cudaPointerAttributes attr;
  cudaError_t err = cudaPointerGetAttributes(&attr, ptr);
  // This function will return invalid value if the pointer is unknown to the
  // cuda runtime. Manually catch this value since it's not really an error.
  if (err == cudaErrorInvalidValue)
  {
    cudaGetLastError(); // Clear the error so we don't raise it later...
    return false;
  }
  SVTKM_CUDA_CALL(err /*= cudaPointerGetAttributes(&attr, ptr)*/);
#if CUDART_VERSION < 10000 // isManaged deprecated in CUDA 10.
  return attr.isManaged != 0;
#else // attr.type doesn't exist before CUDA 10
  return attr.type == cudaMemoryTypeManaged;
#endif
}

void* CudaAllocator::Allocate(std::size_t numBytes)
{
  CudaAllocator::Initialize();
  // When numBytes is zero cudaMallocManaged returns an error and the behavior
  // of cudaMalloc is not documented. Just return nullptr.
  if (numBytes == 0)
  {
    return nullptr;
  }

  void* ptr = nullptr;
  if (ManagedMemoryEnabled)
  {
    SVTKM_CUDA_CALL(cudaMallocManaged(&ptr, numBytes));
  }
  else
  {
    SVTKM_CUDA_CALL(cudaMalloc(&ptr, numBytes));
  }

  {
    SVTKM_LOG_F(svtkm::cont::LogLevel::MemExec,
               "Allocated CUDA array of %s at %p.",
               svtkm::cont::GetSizeString(numBytes).c_str(),
               ptr);
  }

  return ptr;
}

void* CudaAllocator::AllocateUnManaged(std::size_t numBytes)
{
  void* ptr = nullptr;
  SVTKM_CUDA_CALL(cudaMalloc(&ptr, numBytes));
  {
    SVTKM_LOG_F(svtkm::cont::LogLevel::MemExec,
               "Allocated CUDA array of %s at %p.",
               svtkm::cont::GetSizeString(numBytes).c_str(),
               ptr);
  }
  return ptr;
}

void CudaAllocator::Free(void* ptr)
{
  SVTKM_LOG_F(svtkm::cont::LogLevel::MemExec, "Freeing CUDA allocation at %p.", ptr);
  SVTKM_CUDA_CALL(cudaFree(ptr));
}

void CudaAllocator::FreeDeferred(void* ptr, std::size_t numBytes)
{
  static std::mutex deferredMutex;
  static std::vector<void*> deferredPointers;
  static std::size_t deferredSize = 0;
  constexpr std::size_t bufferLimit = 2 << 24; //16MB buffer

  {
    SVTKM_LOG_F(svtkm::cont::LogLevel::MemExec,
               "Deferring free of CUDA allocation at %p of %s.",
               ptr,
               svtkm::cont::GetSizeString(numBytes).c_str());
  }

  std::vector<void*> toFree;
  // critical section
  {
    std::lock_guard<std::mutex> lock(deferredMutex);
    deferredPointers.push_back(ptr);
    deferredSize += numBytes;
    if (deferredSize >= bufferLimit)
    {
      toFree.swap(deferredPointers);
      deferredSize = 0;
    }
  }

  for (auto&& p : toFree)
  {
    SVTKM_LOG_F(svtkm::cont::LogLevel::MemExec, "Freeing deferred CUDA allocation at %p.", p);
    SVTKM_CUDA_CALL(cudaFree(p));
  }
}

void CudaAllocator::PrepareForControl(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    // TODO these hints need to be benchmarked and adjusted once we start
    // sharing the pointers between cont/exec
    SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, cudaCpuDeviceId));
    SVTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, cudaCpuDeviceId, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::PrepareForInput(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    int dev;
    SVTKM_CUDA_CALL(cudaGetDevice(&dev));
    // SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetReadMostly, dev));
    SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    SVTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::PrepareForOutput(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    int dev;
    SVTKM_CUDA_CALL(cudaGetDevice(&dev));
    // SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseUnsetReadMostly, dev));
    SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    SVTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::PrepareForInPlace(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    int dev;
    SVTKM_CUDA_CALL(cudaGetDevice(&dev));
    // SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseUnsetReadMostly, dev));
    SVTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    SVTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::Initialize()
{
#if CUDART_VERSION >= 8000
  std::call_once(IsInitialized, []() {
    int numDevices;
    SVTKM_CUDA_CALL(cudaGetDeviceCount(&numDevices));

    if (numDevices == 0)
    {
      return;
    }

    // Check all devices, use the feature set supported by all
    bool managedMemorySupported = true;
    cudaDeviceProp prop;
    for (int i = 0; i < numDevices && managedMemorySupported; ++i)
    {
      SVTKM_CUDA_CALL(cudaGetDeviceProperties(&prop, i));
      // We check for concurrentManagedAccess, as devices with only the
      // managedAccess property have extra synchronization requirements.
      managedMemorySupported = managedMemorySupported && prop.concurrentManagedAccess;
    }

    HardwareSupportsManagedMemory = managedMemorySupported;
    ManagedMemoryEnabled = managedMemorySupported;

    SVTKM_LOG_F(svtkm::cont::LogLevel::Info,
               "CudaAllocator hardware %s managed memory",
               HardwareSupportsManagedMemory ? "supports" : "doesn't support");

// Check if users want to disable managed memory
#pragma warning(push)
// getenv is not thread safe on windows but since it's inside a call_once block so
// it's fine to suppress the warning here.
#pragma warning(disable : 4996)
    const char* buf = std::getenv(NO_SVTKM_MANAGED_MEMORY);
#pragma warning(pop)
    if (managedMemorySupported && buf != nullptr)
    { //only makes sense to disable managed memory if the hardware supports it
      //in the first place
      ManagedMemoryEnabled = false;
      SVTKM_LOG_F(
        svtkm::cont::LogLevel::Info,
        "CudaAllocator disabling managed memory due to NO_SVTKM_MANAGED_MEMORY env variable");
    }
  });
#endif
}
}
}
}
} // end namespace svtkm::cont::cuda::internal
