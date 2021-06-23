//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef svtkm_worklet_contourtree_augmented_mesh_dem_simlated_simplicity_comperator_h
#define svtkm_worklet_contourtree_augmented_mesh_dem_simlated_simplicity_comperator_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ExecutionObjectBase.h>

namespace svtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem
{


// comparator used for initial sort of data values
template <typename T, typename StorageType, typename DeviceAdapter>
class SimulatedSimplicityIndexComparatorImpl
{
public:
  using ValuePortalType = typename svtkm::cont::ArrayHandle<T, StorageType>::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

  ValuePortalType values;

  // constructor - takes vectors as parameters
  SVTKM_CONT
  SimulatedSimplicityIndexComparatorImpl(ValuePortalType Values)
    : values(Values)
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  SVTKM_EXEC
  bool operator()(const svtkm::Id& i, const svtkm::Id& j) const
  { // operator()
    // value comparison
    if (values.Get(i) < values.Get(j))
      return true;
    if (values.Get(j) < values.Get(i))
      return false;
    // index comparison for simulated simplicity
    if (i < j)
      return true;
    if (j < i)
      return false;
    // fallback - always false
    return false;
  } // operator()*/
};  // SimulatedSimplicityIndexComparatorImpl

template <typename T, typename StorageType>
class SimulatedSimplicityIndexComparator : public svtkm::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  SVTKM_CONT
  SimulatedSimplicityIndexComparator(svtkm::cont::ArrayHandle<T, StorageType> values)
    : Values(values)
  { // constructor
  } // constructor

  template <typename DeviceAdapter>
  SVTKM_CONT SimulatedSimplicityIndexComparatorImpl<T, StorageType, DeviceAdapter>
  PrepareForExecution(DeviceAdapter device) const
  {
    return SimulatedSimplicityIndexComparatorImpl<T, StorageType, DeviceAdapter>(
      this->Values.PrepareForInput(device));
  }

private:
  svtkm::cont::ArrayHandle<T, StorageType> Values;
}; // SimulatedSimplicityIndexComparator

} // namespace mesh_dem_triangulation_worklets
} // namespace contourtree_augmented
} // namespace worklet
} // namespace svtkm

#endif
