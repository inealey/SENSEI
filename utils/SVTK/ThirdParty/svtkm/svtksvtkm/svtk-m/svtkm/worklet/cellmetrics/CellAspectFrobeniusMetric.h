//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef svtk_m_worklet_cellmetrics_CellAspectFrobeniusMetric_h
#define svtk_m_worklet_cellmetrics_CellAspectFrobeniusMetric_h

/*
* Mesh quality metric functions that compute the aspect frobenius of certain mesh cells.
* The aspect frobenius metric generally measures the degree of regularity of a cell, with
* a value of 1 representing a regular cell..
*
* These metric computations are adapted from the SVTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
*
* See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: svtk/ThirdParty/verdict/svtkverdict (for SVTK code implementation of this metric)
*/

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
using FloatType = svtkm::FloatDefault;

// ========================= Unsupported cells ==================================

// By default, cells have undefined aspect frobenius unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            CellShapeType shape,
                                            const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  worklet.RaiseError(
    "Shape type template must be specialized to compute the aspect frobenius metric.");
  return OutType(0.0);
} //If the polygon has 3 vertices or 4 vertices, then just call
//the functions for Triangle and Quad cell types. Otherwise,
//this metric is not supported for (n>4)-vertex polygons, such
//as pentagons or hexagons, or (n<3)-vertex polygons, such as lines or points.
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagPolygon,
                                            const svtkm::exec::FunctorBase& worklet)
{
  if (numPts == 3)
    return CellAspectFrobeniusMetric<OutType>(numPts, pts, svtkm::CellShapeTagTriangle(), worklet);
  else
  {
    worklet.RaiseError(
      "Aspect frobenius metric is not supported for (n<3)- or  (n>4)-vertex polygons.");
    return OutType(0.0);
  }
} //The aspect frobenius metric is not supported for lines/edges.
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagLine,
                                            const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  worklet.RaiseError("Aspect frobenius metric is not supported for lines/edges.");
  return OutType(0.0);
} //The aspect frobenius metric is not uniquely defined for quads.
//Instead, use either the mean or max aspect frobenius metrics, which are
//defined in terms of the aspect frobenius of triangles.
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagQuad,
                                            const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  worklet.RaiseError("Aspect frobenius metric is not supported for quads.");
  return OutType(0.0);
} //The aspect frobenius metric is not uniquely defined for hexahedrons.
//Instead, use either the mean or max aspect frobenius metrics, which are
//defined in terms of the aspect frobenius of tetrahedrons.
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagHexahedron,
                                            const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  worklet.RaiseError("Aspect frobenius metric is not supported for hexahedrons.");
  return OutType(0.0);
} //The aspect frobenius metric is not uniquely defined for pyramids.
//Instead, use either the mean or max aspect frobenius metrics, which are
//defined in terms of the aspect frobenius of tetrahedrons.
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagPyramid,
                                            const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  worklet.RaiseError("Aspect frobenius metric is not supported for pyramids.");
  return OutType(0.0);
} //The aspect frobenius metric is not uniquely defined for wedges.
//Instead, use either the mean or max aspect frobenius metrics, which are
//defined in terms of the aspect frobenius of tetrahedrons.
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagWedge,
                                            const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  worklet.RaiseError("Aspect frobenius metric is not supported for wedges.");
  return OutType(0.0);
} // ========================= 2D cells ==================================

// Computes the aspect frobenius of a triangle.
// Formula: Sum of lengths of 3 edges, divided by a multiple of the triangle area.
// Equals 1 for an equilateral unit triangle.
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagTriangle,
                                            const svtkm::exec::FunctorBase& worklet)
{
  if (numPts != 3)
  {
    worklet.RaiseError("Aspect frobenius metric(triangle) requires 3 points.");
    return OutType(0.0);
  }

  //The 3 edges of a triangle
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge TriEdges[3] = { pts[1] - pts[0], pts[2] - pts[1], pts[0] - pts[2] };

  //Sum the length squared of each edge
  FloatType sum = (FloatType)svtkm::MagnitudeSquared(TriEdges[0]) +
    (FloatType)svtkm::MagnitudeSquared(TriEdges[1]) + (FloatType)svtkm::MagnitudeSquared(TriEdges[2]);

  //Compute the length of the cross product of the triangle.
  //The result is twice the area of the triangle.
  FloatType crossLen = (FloatType)svtkm::Magnitude(svtkm::Cross(TriEdges[0], -TriEdges[2]));

  if (crossLen == 0.0)
    return svtkm::Infinity<OutType>();

  OutType aspect_frobenius = (OutType)(sum / (svtkm::Sqrt(3.0) * 2 * crossLen));

  if (aspect_frobenius > 0.0)
    return svtkm::Min(aspect_frobenius, svtkm::Infinity<OutType>());

  return svtkm::Max(aspect_frobenius, svtkm::NegativeInfinity<OutType>());
} // ============================= 3D Volume cells ==================================i

// Computes the aspect frobenius of a tetrahedron.
// Formula: Sum of lengths of 3 edges, divided by a multiple of the triangle area.
// Equals 1 for a right regular tetrahedron (4 equilateral triangles).
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellAspectFrobeniusMetric(const svtkm::IdComponent& numPts,
                                            const PointCoordVecType& pts,
                                            svtkm::CellShapeTagTetra,
                                            const svtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Aspect frobenius metric(tetrahedron) requires 4 points.");
    return OutType(0.0);
  }

  //Two base edges and one vertical edge, used to compute the tet volume
  using Edge = typename PointCoordVecType::ComponentType;

  const Edge TetEdges[3] = {
    pts[1] - pts[0], //Base edge 1
    pts[2] - pts[0], //Base edge 2
    pts[3] - pts[0]  //Vert edge 3
  };

  //Compute the tet volume
  FloatType denominator = (FloatType)svtkm::Dot(TetEdges[0], svtkm::Cross(TetEdges[1], TetEdges[2]));
  denominator *= denominator;
  denominator *= 2.0f;
  const FloatType normal_exp = 1.0f / 3.0f;
  denominator = 3.0f * svtkm::Pow(denominator, normal_exp);

  if (denominator < svtkm::NegativeInfinity<FloatType>())
    return svtkm::Infinity<OutType>();

  FloatType numerator = (FloatType)svtkm::Dot(TetEdges[0], TetEdges[0]);
  numerator += (FloatType)svtkm::Dot(TetEdges[1], TetEdges[1]);
  numerator += (FloatType)svtkm::Dot(TetEdges[2], TetEdges[2]);
  numerator *= 1.5f;
  numerator -= (FloatType)svtkm::Dot(TetEdges[0], TetEdges[1]);
  numerator -= (FloatType)svtkm::Dot(TetEdges[0], TetEdges[2]);
  numerator -= (FloatType)svtkm::Dot(TetEdges[1], TetEdges[2]);

  OutType aspect_frobenius = (OutType)(numerator / denominator);

  if (aspect_frobenius > 0.0)
    return svtkm::Min(aspect_frobenius, svtkm::Infinity<OutType>());

  return svtkm::Max(aspect_frobenius, svtkm::NegativeInfinity<OutType>());
}
} // namespace cellmetrics
} // namespace worklet
} // namespace svtkm
#endif // svtk_m_worklet_cellmetrics_CellAspectFrobeniusMetric_h
