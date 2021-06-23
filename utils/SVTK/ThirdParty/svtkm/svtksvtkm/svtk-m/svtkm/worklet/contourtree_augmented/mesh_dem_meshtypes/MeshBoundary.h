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

// This header contains a collection of classes used to describe the boundary
// of a mesh, for each main mesh type (i.e., 2D, 3D, and ContourTreeMesh).
// For each mesh type, there are two classes, the actual boundary desriptor
// class and an ExectionObject class with the PrepareForInput function that
// SVTKm expects to generate the object for the execution environment.

#ifndef svtkm_worklet_contourtree_augmented_mesh_boundary_h
#define svtkm_worklet_contourtree_augmented_mesh_boundary_h

#include <cstdlib>

#include <svtkm/worklet/contourtree_augmented/Types.h>
#include <svtkm/worklet/contourtree_augmented/mesh_dem/MeshStructure2D.h>
#include <svtkm/worklet/contourtree_augmented/mesh_dem/MeshStructure3D.h>

#include <svtkm/cont/ExecutionObjectBase.h>

namespace svtkm
{
namespace worklet
{
namespace contourtree_augmented
{


template <typename DeviceTag>
class MeshBoundary2D
{
public:
  // Sort indicies types
  using sortOrderPortalType = typename IdArrayType::template ExecutionTypes<DeviceTag>::PortalConst;

  SVTKM_EXEC_CONT
  MeshBoundary2D()
    : MeshStructure(mesh_dem::MeshStructure2D<DeviceTag>(0, 0))
  {
  }

  SVTKM_CONT
  MeshBoundary2D(svtkm::Id nrows, svtkm::Id ncols, const IdArrayType& sortOrder)
    : MeshStructure(mesh_dem::MeshStructure2D<DeviceTag>(nrows, ncols))
  {
    sortOrderPortal = sortOrder.PrepareForInput(DeviceTag());
  }

  SVTKM_EXEC_CONT
  bool liesOnBoundary(const svtkm::Id index) const
  {
    svtkm::Id meshSortOrderValue = this->sortOrderPortal.Get(index);
    const svtkm::Id row = MeshStructure.vertexRow(meshSortOrderValue);
    const svtkm::Id col = MeshStructure.vertexColumn(meshSortOrderValue);

    return (row == 0) || (col == 0) || (row == MeshStructure.nRows - 1) ||
      (col == MeshStructure.nCols - 1);
  }

private:
  // 2D Mesh size parameters
  mesh_dem::MeshStructure2D<DeviceTag> MeshStructure;
  sortOrderPortalType sortOrderPortal;
};

class MeshBoundary2DExec : public svtkm::cont::ExecutionObjectBase
{
public:
  SVTKM_EXEC_CONT
  MeshBoundary2DExec(svtkm::Id nrows, svtkm::Id ncols, const IdArrayType& inSortOrder)
    : nRows(nrows)
    , nCols(ncols)
    , sortOrder(inSortOrder)
  {
  }

  SVTKM_CONT
  template <typename DeviceTag>
  MeshBoundary2D<DeviceTag> PrepareForExecution(DeviceTag) const
  {
    return MeshBoundary2D<DeviceTag>(this->nRows, this->nCols, this->sortOrder);
  }

private:
  // 2D Mesh size parameters
  svtkm::Id nRows;
  svtkm::Id nCols;
  const IdArrayType& sortOrder;
};


template <typename DeviceTag>
class MeshBoundary3D : public svtkm::cont::ExecutionObjectBase
{
public:
  // Sort indicies types
  using sortOrderPortalType = typename IdArrayType::template ExecutionTypes<DeviceTag>::PortalConst;

  SVTKM_EXEC_CONT
  MeshBoundary3D()
    : MeshStructure(mesh_dem::MeshStructure3D<DeviceTag>(0, 0, 0))
  {
  }

  SVTKM_CONT
  MeshBoundary3D(svtkm::Id nrows, svtkm::Id ncols, svtkm::Id nslices, const IdArrayType& sortOrder)
    : MeshStructure(mesh_dem::MeshStructure3D<DeviceTag>(nrows, ncols, nslices))
  {
    sortOrderPortal = sortOrder.PrepareForInput(DeviceTag());
  }

  SVTKM_EXEC_CONT
  bool liesOnBoundary(const svtkm::Id index) const
  {
    svtkm::Id meshSortOrderValue = this->sortOrderPortal.Get(index);
    const svtkm::Id row = MeshStructure.vertexRow(meshSortOrderValue);
    const svtkm::Id col = MeshStructure.vertexColumn(meshSortOrderValue);
    const svtkm::Id sli = MeshStructure.vertexSlice(meshSortOrderValue);
    return (row == 0) || (col == 0) || (sli == 0) || (row == MeshStructure.nRows - 1) ||
      (col == MeshStructure.nCols - 1) || (sli == MeshStructure.nSlices - 1);
  }

protected:
  // 3D Mesh size parameters
  mesh_dem::MeshStructure3D<DeviceTag> MeshStructure;
  sortOrderPortalType sortOrderPortal;
};


class MeshBoundary3DExec : public svtkm::cont::ExecutionObjectBase
{
public:
  SVTKM_EXEC_CONT
  MeshBoundary3DExec(svtkm::Id nrows,
                     svtkm::Id ncols,
                     svtkm::Id nslices,
                     const IdArrayType& inSortOrder)
    : nRows(nrows)
    , nCols(ncols)
    , nSlices(nslices)
    , sortOrder(inSortOrder)
  {
  }

  SVTKM_CONT
  template <typename DeviceTag>
  MeshBoundary3D<DeviceTag> PrepareForExecution(DeviceTag) const
  {
    return MeshBoundary3D<DeviceTag>(this->nRows, this->nCols, this->nSlices, this->sortOrder);
  }

protected:
  // 3D Mesh size parameters
  svtkm::Id nRows;
  svtkm::Id nCols;
  svtkm::Id nSlices;
  const IdArrayType& sortOrder;
};


template <typename DeviceTag>
class MeshBoundaryContourTreeMesh
{
public:
  using IndicesPortalType = typename IdArrayType::template ExecutionTypes<DeviceTag>::PortalConst;

  SVTKM_EXEC_CONT
  MeshBoundaryContourTreeMesh() {}

  SVTKM_CONT
  MeshBoundaryContourTreeMesh(const IdArrayType& globalMeshIndex,
                              svtkm::Id totalNRows,
                              svtkm::Id totalNCols,
                              svtkm::Id3 minIdx,
                              svtkm::Id3 maxIdx)
    : TotalNRows(totalNRows)
    , TotalNCols(totalNCols)
    , MinIdx(minIdx)
    , MaxIdx(maxIdx)
  {
    assert(TotalNRows > 0 && TotalNCols > 0);
    this->GlobalMeshIndexPortal = globalMeshIndex.PrepareForInput(DeviceTag());
  }

  SVTKM_EXEC_CONT
  bool liesOnBoundary(const svtkm::Id index) const
  {
    svtkm::Id idx = GlobalMeshIndexPortal.Get(index);
    svtkm::Id3 rcs;
    rcs[0] = svtkm::Id((idx % (this->TotalNRows * this->TotalNCols)) / this->TotalNCols);
    rcs[1] = svtkm::Id(idx % this->TotalNCols);
    rcs[2] = svtkm::Id(idx / (this->TotalNRows * this->TotalNCols));
    for (int d = 0; d < 3; ++d)
    {
      if (this->MinIdx[d] != this->MaxIdx[d] &&
          (rcs[d] == this->MinIdx[d] || rcs[d] == this->MaxIdx[d]))
      {
        return true;
      }
    }
    return false;
  }

private:
  // mesh block parameters
  svtkm::Id TotalNRows;
  svtkm::Id TotalNCols;
  svtkm::Id3 MinIdx;
  svtkm::Id3 MaxIdx;
  IndicesPortalType GlobalMeshIndexPortal;
};


class MeshBoundaryContourTreeMeshExec : public svtkm::cont::ExecutionObjectBase
{
public:
  SVTKM_EXEC_CONT
  MeshBoundaryContourTreeMeshExec(const IdArrayType& globalMeshIndex,
                                  svtkm::Id totalNRows,
                                  svtkm::Id totalNCols,
                                  svtkm::Id3 minIdx,
                                  svtkm::Id3 maxIdx)
    : GlobalMeshIndex(globalMeshIndex)
    , TotalNRows(totalNRows)
    , TotalNCols(totalNCols)
    , MinIdx(minIdx)
    , MaxIdx(maxIdx)
  {
  }

  SVTKM_CONT
  template <typename DeviceTag>
  MeshBoundaryContourTreeMesh<DeviceTag> PrepareForExecution(DeviceTag) const
  {
    return MeshBoundaryContourTreeMesh<DeviceTag>(
      GlobalMeshIndex, this->TotalNRows, this->TotalNCols, this->MinIdx, this->MaxIdx);
  }

private:
  const IdArrayType& GlobalMeshIndex;
  svtkm::Id TotalNRows;
  svtkm::Id TotalNCols;
  svtkm::Id3 MinIdx;
  svtkm::Id3 MaxIdx;
};


} // namespace contourtree_augmented
} // worklet
} // svtkm

#endif
