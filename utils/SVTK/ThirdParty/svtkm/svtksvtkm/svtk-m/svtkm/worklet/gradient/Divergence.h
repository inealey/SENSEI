//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_gradient_Divergence_h
#define svtk_m_worklet_gradient_Divergence_h

#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace worklet
{
namespace gradient
{

using DivergenceTypes = svtkm::List<svtkm::Vec<svtkm::Vec3f_32, 3>, svtkm::Vec<svtkm::Vec3f_64, 3>>;


struct Divergence : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn input, FieldOut output);

  template <typename InputType, typename OutputType>
  SVTKM_EXEC void operator()(const InputType& input, OutputType& divergence) const
  {
    divergence = input[0][0] + input[1][1] + input[2][2];
  }
};
}
}
}
#endif
