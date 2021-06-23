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

#ifndef svtkm_worklet_contourtree_augmented_mesh_dem_execution_object_mesh_3d_h
#define svtkm_worklet_contourtree_augmented_mesh_dem_execution_object_mesh_3d_h

#include <svtkm/Types.h>


namespace svtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem
{

// Worklet for computing the sort indices from the sort order
template <typename DeviceAdapter>
class MeshStructure3D
{
public:
  SVTKM_EXEC_CONT
  MeshStructure3D()
    : nCols(0)
    , nRows(0)
    , nSlices(0)
  {
  }

  SVTKM_EXEC_CONT
  MeshStructure3D(svtkm::Id ncols, svtkm::Id nrows, svtkm::Id nslices)
    : nCols(ncols)
    , nRows(nrows)
    , nSlices(nslices)
  {
  }

  // number of mesh vertices
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfVertices() const { return (this->nRows * this->nCols * this->nSlices); }

  // vertex row - integer modulus by (nRows&nCols) and integer divide by columns
  SVTKM_EXEC
  svtkm::Id vertexRow(svtkm::Id v) const { return (v % (nRows * nCols)) / nCols; }

  // vertex column -- integer modulus by columns
  SVTKM_EXEC
  svtkm::Id vertexColumn(svtkm::Id v) const { return v % nCols; }

  // vertex slice -- integer divide by (nRows*nCols)
  SVTKM_EXEC
  svtkm::Id vertexSlice(svtkm::Id v) const { return v / (nRows * nCols); }

  //vertex ID - row * ncols + col
  SVTKM_EXEC
  svtkm::Id vertexId(svtkm::Id s, svtkm::Id r, svtkm::Id c) const
  {
    return (s * nRows + r) * nCols + c;
  }

  svtkm::Id nCols, nRows, nSlices;

}; // Mesh_DEM_2D_ExecutionObject

} // namespace mesh_dem_triangulation_worklets
} // namespace contourtree_augmented
} // namespace worklet
} // namespace svtkm

#endif
