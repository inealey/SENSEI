//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchExtrude_h
#define svtk_m_exec_arg_FetchExtrude_h

#include <svtkm/exec/ConnectivityExtrude.h>
#include <svtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <svtkm/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <svtkm/exec/arg/IncidentElementIndices.h>

//optimized fetches for ArrayPortalExtrude for
// - 3D Scheduling
// - WorkletNeighboorhood
namespace svtkm
{
namespace exec
{
namespace arg
{

//Optimized fetch for point ids when iterating the cells ConnectivityExtrude
template <typename FetchType, typename Device, typename ExecObjectType>
struct Fetch<FetchType,
             svtkm::exec::arg::AspectTagIncidentElementIndices,
             svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>,
             ExecObjectType>
{
  using ThreadIndicesType =
    svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>;

  using ValueType = svtkm::Vec<svtkm::Id, 6>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    // std::cout << "opimized fetch for point ids" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const svtkm::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const svtkm::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);
    ValueType result;
    result[0] = offset1 + xgcidx.PointIds[0][0];
    result[1] = offset1 + xgcidx.PointIds[0][1];
    result[2] = offset1 + xgcidx.PointIds[0][2];
    result[3] = offset2 + xgcidx.PointIds[1][0];
    result[4] = offset2 + xgcidx.PointIds[1][1];
    result[5] = offset2 + xgcidx.PointIds[1][2];
    return result;
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};

//Optimized fetch for point arrays when iterating the cells ConnectivityExtrude
template <typename Device, typename PortalType>
struct Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>,
             PortalType>
{
  using ThreadIndicesType =
    svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>;
  using ValueType = svtkm::Vec<typename PortalType::ValueType, 6>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const PortalType& portal)
  {
    // std::cout << "opimized fetch for point values" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const svtkm::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const svtkm::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);
    ValueType result;
    result[0] = portal.Get(offset1 + xgcidx.PointIds[0][0]);
    result[1] = portal.Get(offset1 + xgcidx.PointIds[0][1]);
    result[2] = portal.Get(offset1 + xgcidx.PointIds[0][2]);
    result[3] = portal.Get(offset2 + xgcidx.PointIds[1][0]);
    result[4] = portal.Get(offset2 + xgcidx.PointIds[1][1]);
    result[5] = portal.Get(offset2 + xgcidx.PointIds[1][2]);
    return result;
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, const PortalType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the cells of ConnectivityExtrude
template <typename Device, typename T>
struct Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>,
             svtkm::exec::ArrayPortalExtrude<T>>

{
  using ThreadIndicesType =
    svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>;
  using ValueType = svtkm::Vec<typename svtkm::exec::ArrayPortalExtrude<T>::ValueType, 6>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const svtkm::exec::ArrayPortalExtrude<T>& points)
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return points.GetWedge(indices.GetIndicesIncident());
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&,
             const svtkm::exec::ArrayPortalExtrude<T>&,
             const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the cells of ConnectivityExtrude
template <typename Device, typename T>
struct Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>,
             svtkm::exec::ArrayPortalExtrudePlane<T>>
{
  using ThreadIndicesType =
    svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ConnectivityExtrude<Device>>;
  using ValueType = svtkm::Vec<typename svtkm::exec::ArrayPortalExtrudePlane<T>::ValueType, 6>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices,
                 const svtkm::exec::ArrayPortalExtrudePlane<T>& portal)
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return portal.GetWedge(indices.GetIndicesIncident());
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&,
             const svtkm::exec::ArrayPortalExtrudePlane<T>&,
             const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the points of ConnectivityExtrude
template <typename Device, typename T>
struct Fetch<
  svtkm::exec::arg::FetchTagArrayDirectIn,
  svtkm::exec::arg::AspectTagDefault,
  svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ReverseConnectivityExtrude<Device>>,
  svtkm::exec::ArrayPortalExtrude<T>>

{
  using ThreadIndicesType =
    svtkm::exec::arg::ThreadIndicesTopologyMap<svtkm::exec::ReverseConnectivityExtrude<Device>>;
  using ValueType = typename svtkm::exec::ArrayPortalExtrude<T>::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const svtkm::exec::ArrayPortalExtrude<T>& points)
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return points.Get(indices.GetIndexLogical());
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&,
             const svtkm::exec::ArrayPortalExtrude<T>&,
             const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
}


#endif
