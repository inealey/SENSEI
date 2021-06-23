/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkConeSource.h"

#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"

#include <cmath>

svtkStandardNewMacro(svtkConeSource);

//----------------------------------------------------------------------------
// Construct with default resolution 6, height 1.0, radius 0.5, and capping
// on.
svtkConeSource::svtkConeSource(int res)
{
  res = (res < 0 ? 0 : res);
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Direction[0] = 1.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 0.0;

  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int svtkConeSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  double angle;
  int numLines, numPolys, numPts;
  double x[3], xbot;
  int i;
  svtkIdType pts[SVTK_CELL_SIZE];
  svtkPoints* newPoints;
  svtkCellArray* newLines = nullptr;
  svtkCellArray* newPolys = nullptr;
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  // for streaming
  int piece;
  int numPieces;
  int maxPieces;
  int start, end;
  int createBottom;

  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (piece >= this->Resolution && !(piece == 0 && this->Resolution == 0))
  {
    return 1;
  }
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  maxPieces = this->Resolution != 0 ? this->Resolution : 1;
  if (numPieces > maxPieces)
  {
    numPieces = maxPieces;
  }
  if (piece >= maxPieces)
  {
    // Super class should do this for us,
    // but I put this condition in any way.
    return 1;
  }
  start = maxPieces * piece / numPieces;
  end = (maxPieces * (piece + 1) / numPieces) - 1;
  createBottom = (this->Capping && (start == 0));

  svtkDebugMacro("ConeSource Executing");

  if (this->Resolution)
  {
    angle = 2.0 * svtkMath::Pi() / this->Resolution;
  }
  else
  {
    angle = 0.0;
  }

  // Set things up; allocate memory
  //
  switch (this->Resolution)
  {
    case 0:
      numPts = 2;
      numLines = 1;
      newLines = svtkCellArray::New();
      newLines->AllocateEstimate(numLines, numPts);
      break;

    case 1:
    case 2:
      numPts = 2 * this->Resolution + 1;
      numPolys = this->Resolution;
      newPolys = svtkCellArray::New();
      newPolys->AllocateEstimate(numPolys, 3);
      break;

    default:
      if (createBottom)
      {
        // piece 0 has cap.
        numPts = this->Resolution + 1;
        numPolys = end - start + 2;
      }
      else
      {
        numPts = end - start + 3;
        numPolys = end - start + 2;
      }
      newPolys = svtkCellArray::New();
      newPolys->AllocateEstimate(numPolys, this->Resolution);
      break;
  }
  newPoints = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }

  newPoints->Allocate(numPts);

  // Create cone
  //
  x[0] = this->Height / 2.0; // zero-centered
  x[1] = 0.0;
  x[2] = 0.0;
  pts[0] = newPoints->InsertNextPoint(x);

  xbot = -this->Height / 2.0;

  switch (this->Resolution)
  {
    case 0:
      x[0] = xbot;
      x[1] = 0.0;
      x[2] = 0.0;
      pts[1] = newPoints->InsertNextPoint(x);
      newLines->InsertNextCell(2, pts);
      break;

    case 2: // fall through this case to use the code in case 1
      x[0] = xbot;
      x[1] = 0.0;
      x[2] = -this->Radius;
      pts[1] = newPoints->InsertNextPoint(x);
      x[0] = xbot;
      x[1] = 0.0;
      x[2] = this->Radius;
      pts[2] = newPoints->InsertNextPoint(x);
      newPolys->InsertNextCell(3, pts);
      SVTK_FALLTHROUGH;

    case 1:
      x[0] = xbot;
      x[1] = -this->Radius;
      x[2] = 0.0;
      pts[1] = newPoints->InsertNextPoint(x);
      x[0] = xbot;
      x[1] = this->Radius;
      x[2] = 0.0;
      pts[2] = newPoints->InsertNextPoint(x);
      newPolys->InsertNextCell(3, pts);
      break;

    default: // General case: create Resolution triangles and single cap
      // create the bottom.
      if (createBottom)
      {
        for (i = 0; i < this->Resolution; i++)
        {
          x[0] = xbot;
          x[1] = this->Radius * cos(i * angle);
          x[2] = this->Radius * sin(i * angle);
          // Reverse the order
          pts[this->Resolution - i - 1] = newPoints->InsertNextPoint(x);
        }
        newPolys->InsertNextCell(this->Resolution, pts);
      }

      pts[0] = 0;
      if (!createBottom)
      {
        // we need to create the points also
        x[0] = xbot;
        x[1] = this->Radius * cos(start * angle);
        x[2] = this->Radius * sin(start * angle);
        pts[1] = newPoints->InsertNextPoint(x);
        for (i = start; i <= end; ++i)
        {
          x[1] = this->Radius * cos((i + 1) * angle);
          x[2] = this->Radius * sin((i + 1) * angle);
          pts[2] = newPoints->InsertNextPoint(x);
          newPolys->InsertNextCell(3, pts);
          pts[1] = pts[2];
        }
      }
      else
      {
        // bottom and points have already been created.
        for (i = start; i <= end; i++)
        {
          pts[1] = i + 1;
          pts[2] = i + 2;
          if (pts[2] > this->Resolution)
          {
            pts[2] = 1;
          }
          newPolys->InsertNextCell(3, pts);
        }
      } // createBottom

  } // switch

  // A non-default origin and/or direction requires transformation
  //
  if (this->Center[0] != 0.0 || this->Center[1] != 0.0 || this->Center[2] != 0.0 ||
    this->Direction[0] != 1.0 || this->Direction[1] != 0.0 || this->Direction[2] != 0.0)
  {
    svtkTransform* t = svtkTransform::New();
    t->Translate(this->Center[0], this->Center[1], this->Center[2]);
    double vMag = svtkMath::Norm(this->Direction);
    if (this->Direction[0] < 0.0)
    {
      // flip x -> -x to avoid instability
      t->RotateWXYZ(180.0, (this->Direction[0] - vMag) / 2.0, this->Direction[1] / 2.0,
        this->Direction[2] / 2.0);
      t->RotateWXYZ(180.0, 0, 1, 0);
    }
    else
    {
      t->RotateWXYZ(180.0, (this->Direction[0] + vMag) / 2.0, this->Direction[1] / 2.0,
        this->Direction[2] / 2.0);
    }
    if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
    {
      double* ipts = static_cast<svtkDoubleArray*>(newPoints->GetData())->GetPointer(0);
      for (i = 0; i < numPts; i++, ipts += 3)
      {
        t->TransformPoint(ipts, ipts);
      }
    }
    else
    {
      float* ipts = static_cast<svtkFloatArray*>(newPoints->GetData())->GetPointer(0);
      for (i = 0; i < numPts; i++, ipts += 3)
      {
        t->TransformPoint(ipts, ipts);
      }
    }

    t->Delete();
  }

  // Update ourselves
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newPolys)
  {
    newPolys->Squeeze(); // we may have estimated size; reclaim some space
    output->SetPolys(newPolys);
    newPolys->Delete();
  }
  else
  {
    output->SetLines(newLines);
    newLines->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkConeSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void svtkConeSource::SetAngle(double angle)
{
  this->SetRadius(this->Height * tan(svtkMath::RadiansFromDegrees(angle)));
}

//----------------------------------------------------------------------------
double svtkConeSource::GetAngle()
{
  return svtkMath::DegreesFromRadians(atan2(this->Radius, this->Height));
}

//----------------------------------------------------------------------------
void svtkConeSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")\n";
  os << indent << "Direction: (" << this->Direction[0] << ", " << this->Direction[1] << ", "
     << this->Direction[2] << ")\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
