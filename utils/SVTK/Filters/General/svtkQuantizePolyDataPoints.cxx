/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQuantizePolyDataPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkQuantizePolyDataPoints.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkQuantizePolyDataPoints);

//--------------------------------------------------------------------------
// Construct object with initial QFactor of 0.25
svtkQuantizePolyDataPoints::svtkQuantizePolyDataPoints()
{
  this->QFactor = 0.25;
  this->Tolerance = 0.0;
}

//--------------------------------------------------------------------------
void svtkQuantizePolyDataPoints::OperateOnPoint(double in[3], double out[3])
{
  out[0] = floor(in[0] / this->QFactor + 0.5) * this->QFactor;
  out[1] = floor(in[1] / this->QFactor + 0.5) * this->QFactor;
  out[2] = floor(in[2] / this->QFactor + 0.5) * this->QFactor;
}

//-------------------------------------------------------------------------
void svtkQuantizePolyDataPoints::OperateOnBounds(double in[6], double out[6])
{
  out[0] = floor(in[0] / this->QFactor + 0.5) * this->QFactor;
  out[1] = floor(in[1] / this->QFactor + 0.5) * this->QFactor;
  out[2] = floor(in[2] / this->QFactor + 0.5) * this->QFactor;
  out[3] = floor(in[3] / this->QFactor + 0.5) * this->QFactor;
  out[4] = floor(in[4] / this->QFactor + 0.5) * this->QFactor;
  out[5] = floor(in[5] / this->QFactor + 0.5) * this->QFactor;
}

//--------------------------------------------------------------------------
void svtkQuantizePolyDataPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "QFactor: " << this->QFactor << "\n";
}
