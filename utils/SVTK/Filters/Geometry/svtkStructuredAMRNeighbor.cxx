/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkStructuredAMRNeighbor.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkStructuredAMRNeighbor.h"
#include "svtkStructuredExtent.h"

svtkStructuredAMRNeighbor::svtkStructuredAMRNeighbor()
{
  this->GridLevel = -1;
  this->NeighborLevel = -1;
  this->NeighborID = 0;
  this->RelationShip = svtkStructuredAMRNeighbor::UNDEFINED;

  for (int i = 0; i < 3; ++i)
  {
    this->Orientation[i] = svtkStructuredNeighbor::UNDEFINED;
    int minIdx = i * 2;
    int maxIdx = i * 2 + 1;
    this->GridOverlapExtent[minIdx] = this->GridOverlapExtent[maxIdx] =
      this->OverlapExtent[minIdx] = this->OverlapExtent[maxIdx] = this->SendExtent[minIdx] =
        this->SendExtent[maxIdx] = this->RcvExtent[minIdx] = this->RcvExtent[maxIdx] = -1;
  } // END for all dimensions
}

//-----------------------------------------------------------------------------
svtkStructuredAMRNeighbor::svtkStructuredAMRNeighbor(const int gridLevel, const int neiID,
  const int neighborLevel, int gridOverlap[6], int neiOverlap[6], int orient[3],
  const int relationShip)
{
  this->GridLevel = gridLevel;
  this->NeighborID = neiID;
  this->NeighborLevel = neighborLevel;
  this->RelationShip = relationShip;

  for (int i = 0; i < 3; ++i)
  {
    int minIdx = i * 2;
    int maxIdx = i * 2 + 1;

    this->RcvExtent[minIdx] = this->OverlapExtent[minIdx] = neiOverlap[minIdx];
    this->RcvExtent[maxIdx] = this->OverlapExtent[maxIdx] = neiOverlap[maxIdx];

    this->SendExtent[minIdx] = this->GridOverlapExtent[minIdx] = gridOverlap[minIdx];
    this->SendExtent[maxIdx] = this->GridOverlapExtent[maxIdx] = gridOverlap[maxIdx];

    this->Orientation[i] = orient[i];
  } // END for all dimensions
}

//-----------------------------------------------------------------------------
svtkStructuredAMRNeighbor& svtkStructuredAMRNeighbor::operator=(const svtkStructuredAMRNeighbor& N)
{
  if (this != &N)
  {
    this->GridLevel = N.GridLevel;
    this->NeighborID = N.NeighborID;
    this->NeighborLevel = N.NeighborLevel;
    this->RelationShip = N.RelationShip;

    for (int i = 0; i < 3; ++i)
    {
      int minIdx = i * 2;
      int maxIdx = i * 2 + 1;

      this->RcvExtent[minIdx] = N.RcvExtent[minIdx];
      this->RcvExtent[maxIdx] = N.RcvExtent[maxIdx];

      this->SendExtent[minIdx] = N.SendExtent[minIdx];
      this->SendExtent[maxIdx] = N.SendExtent[maxIdx];

      this->OverlapExtent[minIdx] = N.OverlapExtent[minIdx];
      this->OverlapExtent[maxIdx] = N.OverlapExtent[maxIdx];

      this->GridOverlapExtent[minIdx] = N.GridOverlapExtent[minIdx];
      this->GridOverlapExtent[maxIdx] = N.GridOverlapExtent[maxIdx];
    } // END for all dimensions
  }   // END if
  return *this;
}

//-----------------------------------------------------------------------------
std::string svtkStructuredAMRNeighbor::GetRelationShipString()
{
  std::string str;
  switch (this->RelationShip)
  {
    case PARENT:
      str = "PARENT";
      break;
    case PARTIALLY_OVERLAPPING_PARENT:
      str = "PARTIALLY_OVERLAPPING_PARENT";
      break;
    case CHILD:
      str = "CHILD";
      break;
    case PARTIALLY_OVERLAPPING_CHILD:
      str = "PARTIALLY_OVERLAPPING_CHILD";
      break;
    case SAME_LEVEL_SIBLING:
      str = "SAME_LEVEL_SIBLING";
      break;
    case COARSE_TO_FINE_SIBLING:
      str = "COARSE_TO_FINE_SIBLING";
      break;
    case FINE_TO_COARSE_SIBLING:
      str = "FINE_TO_COARSE_SIBLING";
      break;
    case UNDEFINED: /* intentional fall-through */
    default:
      str = "UNDEFINED";
  } // END switch

  return (str);
}

//-----------------------------------------------------------------------------
void svtkStructuredAMRNeighbor::GetReceiveExtentOnGrid(const int ng, int gridExtent[6], int ext[6])
{
  for (int i = 0; i < 6; ++i)
  {
    ext[i] = this->GridOverlapExtent[i];
  }

  for (int i = 0; i < 3; ++i)
  {
    switch (this->Orientation[i])
    {
      case svtkStructuredNeighbor::SUPERSET:
        /* NO OP */
        break;
      case svtkStructuredNeighbor::SUBSET_HI:
      case svtkStructuredNeighbor::HI:
        ext[i * 2 + 1] += ng;
        break;
      case svtkStructuredNeighbor::SUBSET_LO:
      case svtkStructuredNeighbor::LO:
        ext[i * 2] -= ng;
        break;
      case svtkStructuredNeighbor::SUBSET_BOTH:
        ext[i * 2] -= ng;
        ext[i * 2 + 1] += ng;
        break;
      default:; /* NO OP */
    }           // END switch
  }             // END for all dimensions
  svtkStructuredExtent::Clamp(ext, gridExtent);
}

//-----------------------------------------------------------------------------
void svtkStructuredAMRNeighbor::ComputeSendAndReceiveExtent(int gridRealExtent[6],
  int* svtkNotUsed(gridGhostedExtent), int neiRealExtent[6], int* svtkNotUsed(WholeExtent),
  const int N)
{

  // TODO: Here we need to make sure that the send/rcv extent between a coarse
  // fine boundary will be as such that ghost layers of the fine grid will cover
  // the entire lower res cell based on the level difference between the
  // grid and its neighbor.

  for (int i = 0; i < 3; ++i)
  {
    switch (this->Orientation[i])
    {
      case svtkStructuredNeighbor::SUPERSET:
        this->SendExtent[i * 2] -= N;
        this->SendExtent[i * 2 + 1] += N;
        break;
      case svtkStructuredNeighbor::SUBSET_HI:
      case svtkStructuredNeighbor::HI:
        this->RcvExtent[i * 2 + 1] += N;
        this->SendExtent[i * 2] -= N;
        break;
      case svtkStructuredNeighbor::SUBSET_LO:
      case svtkStructuredNeighbor::LO:
        this->RcvExtent[i * 2] -= N;
        this->SendExtent[i * 2 + 1] += N;
        break;
      case svtkStructuredNeighbor::SUBSET_BOTH:
        this->RcvExtent[i * 2] -= N;
        this->SendExtent[i * 2 + 1] += N;
        this->RcvExtent[i * 2 + 1] += N;
        this->SendExtent[i * 2] -= N;
        break;
      default:; /* NO OP */
    }           // END switch
  }             // END for all dimensions

  // Hmm...restricting receive extent to the real extent of the neighbor
  svtkStructuredExtent::Clamp(this->RcvExtent, neiRealExtent);
  svtkStructuredExtent::Clamp(this->SendExtent, gridRealExtent);
}
