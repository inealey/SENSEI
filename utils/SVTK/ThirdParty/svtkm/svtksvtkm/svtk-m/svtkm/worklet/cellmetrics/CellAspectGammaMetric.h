//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef svtk_m_worklet_cellmetrics_CellAspectGammaMetric_h
#define svtk_m_worklet_cellmetrics_CellAspectGammaMetric_h

/*
* Mesh quality metric functions that compute the aspect ratio of mesh cells.
** These metric computations are adapted from the SVTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: svtk/ThirdParty/verdict/svtkverdict (for SVTK code implementation of this metric)
*/

#include "TypeOfCellHexahedral.h"
#include "TypeOfCellQuadrilateral.h"
#include "TypeOfCellTetrahedral.h"
#include "TypeOfCellTriangle.h"
#include "svtkm/CellShape.h"
#include "svtkm/CellTraits.h"
#include "svtkm/VecTraits.h"
#include "svtkm/VectorAnalysis.h"
#include "svtkm/exec/FunctorBase.h"
#define UNUSED(expr) (void)(expr);

namespace svtkm
{
namespace worklet
{
namespace cellmetrics
{
// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
SVTKM_EXEC OutType CellAspectGammaMetric(const svtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        CellShapeType shape,
                                        const svtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(0);
}

// ============================= 3D Volume cells ==================================
// Compute the aspect ratio of a tetrahedron.
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectGammaMetric(const svtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        svtkm::CellShapeTagTetra,
                                        const svtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Aspect gamma metric (tetrahedron) requires 4 points.");
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar volume = GetTetraVolume<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar vAbs = svtkm::Abs(volume);

  if (vAbs <= Scalar(0.0))
  {
    return svtkm::Infinity<Scalar>();
  }
  const Scalar six = Scalar(6.0);
  const Scalar l0 = GetTetraL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetTetraL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetTetraL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l3 = GetTetraL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l4 = GetTetraL4Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l5 = GetTetraL5Magnitude<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar R = svtkm::Sqrt((svtkm::Pow(l0, 2) + svtkm::Pow(l1, 2) + svtkm::Pow(l2, 2) +
                               svtkm::Pow(l3, 2) + svtkm::Pow(l4, 2) + svtkm::Pow(l5, 2)) /
                              six);

  const Scalar rootTwo(svtkm::Sqrt(Scalar(2.0)));
  const Scalar twelve(12.0);
  const Scalar q = (svtkm::Pow(R, 3) * rootTwo) / (twelve * vAbs);

  return q;
}

} // namespace cellmetrics
} // namespace worklet
} // namespace svtkm
#endif // svtk_m_worklet_cellmetrics_CellAspectGammaMetric_h
