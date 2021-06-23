/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSubPixelPositionEdgels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSubPixelPositionEdgels.h"

#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredPoints.h"

svtkStandardNewMacro(svtkSubPixelPositionEdgels);

svtkSubPixelPositionEdgels::svtkSubPixelPositionEdgels()
{
  this->TargetFlag = 0;
  this->TargetValue = 0.0;

  this->SetNumberOfInputPorts(2);
}

svtkSubPixelPositionEdgels::~svtkSubPixelPositionEdgels() = default;

int svtkSubPixelPositionEdgels::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* gradMapsInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkStructuredPoints* gradMaps =
    svtkStructuredPoints::SafeDownCast(gradMapsInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPoints* newPts;
  svtkDoubleArray* newNormals;
  svtkPoints* inPts;
  svtkDataArray* inVectors;
  svtkIdType ptId;
  float* MapData = nullptr;
  double* DMapData = nullptr;
  double pnt[3];
  int* dimensions;
  double result[3], resultNormal[3];
  double *spacing, *origin;

  svtkDebugMacro(<< "SubPixelPositioning Edgels");

  if (numPts < 1 || (inPts = input->GetPoints()) == nullptr)
  {
    svtkErrorMacro(<< "No data to fit!");
    return 1;
  }

  newPts = svtkPoints::New();
  newNormals = svtkDoubleArray::New();
  newNormals->SetNumberOfComponents(3);

  dimensions = gradMaps->GetDimensions();
  spacing = gradMaps->GetSpacing();
  origin = gradMaps->GetOrigin();
  if (svtkArrayDownCast<svtkDoubleArray>(gradMaps->GetPointData()->GetScalars()))
  {
    DMapData =
      svtkArrayDownCast<svtkDoubleArray>(gradMaps->GetPointData()->GetScalars())->GetPointer(0);
  }
  else if (svtkArrayDownCast<svtkFloatArray>(gradMaps->GetPointData()->GetScalars()))
  {
    MapData =
      svtkArrayDownCast<svtkFloatArray>(gradMaps->GetPointData()->GetScalars())->GetPointer(0);
  }
  else
  {
    svtkErrorMacro(<< "Point data must be float or double!");
    return 1;
  }

  inVectors = gradMaps->GetPointData()->GetVectors();

  //
  // Loop over all points, adjusting locations
  //
  for (ptId = 0; ptId < inPts->GetNumberOfPoints(); ptId++)
  {
    inPts->GetPoint(ptId, pnt);
    pnt[0] = (pnt[0] - origin[0]) / spacing[0];
    pnt[1] = (pnt[1] - origin[1]) / spacing[1];
    pnt[2] = (pnt[2] - origin[2]) / spacing[2];
    if (MapData)
    {
      this->Move(dimensions[0], dimensions[1], dimensions[2], (int)(pnt[0] + 0.5),
        (int)(pnt[1] + 0.5), MapData, inVectors, result, (int)(pnt[2] + 0.5), spacing,
        resultNormal);
    }
    else if (DMapData)
    {
      this->Move(dimensions[0], dimensions[1], dimensions[2], (int)(pnt[0] + 0.5),
        (int)(pnt[1] + 0.5), DMapData, inVectors, result, (int)(pnt[2] + 0.5), spacing,
        resultNormal);
    }
    result[0] = result[0] * spacing[0] + origin[0];
    result[1] = result[1] * spacing[1] + origin[1];
    result[2] = result[2] * spacing[2] + origin[2];
    newPts->InsertNextPoint(result);
    newNormals->InsertNextTuple(resultNormal);
  }

  output->CopyStructure(input);
  output->GetPointData()->CopyNormalsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->SetNormals(newNormals);
  output->SetPoints(newPts);
  newPts->Delete();
  newNormals->Delete();

  return 1;
}

void svtkSubPixelPositionEdgels::Move(int xdim, int ydim, int zdim, int x, int y, double* img,
  svtkDataArray* inVecs, double* result, int z, double* spacing, double* resultNormal)
{
  int ypos;
  svtkIdType zpos;
  double vec[3];
  double valn, valp;
  double mag;
  double a, b, c;
  double xp, yp, zp;
  double xn, yn, zn;
  int xi, yi, zi, i;

  zpos = z * xdim * ydim;

  ypos = y * xdim;

  // handle the 2d case
  if (zdim < 2)
  {
    if (x < 1 || y < 1 || x >= (xdim - 2) || y >= (ydim - 2))
    {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      // if the point of off of the grad map just make up a value
      if (x < 0 || y < 0 || x > xdim || y > ydim)
      {
        resultNormal[0] = 1;
        resultNormal[1] = 0;
        resultNormal[2] = 0;
      }
      else
      {
        for (i = 0; i < 3; i++)
        {
          resultNormal[i] = inVecs->GetTuple(x + xdim * y)[i];
        }
        svtkMath::Normalize(resultNormal);
      }
    }
    else
    {
      // first get the orientation
      inVecs->GetTuple(x + ypos, vec);
      vec[0] = vec[0] * spacing[0];
      vec[1] = vec[1] * spacing[1];
      vec[2] = 0;
      svtkMath::Normalize(vec);
      mag = img[x + ypos];

      // compute the sample points
      xp = (double)x + vec[0];
      yp = (double)y + vec[1];
      xn = (double)x - vec[0];
      yn = (double)y - vec[1];

      // compute their values
      xi = (int)xp;
      yi = (int)yp;
      valp = img[xi + xdim * yi] * (1.0 - xp + xi) * (1.0 - yp + yi) +
        img[1 + xi + xdim * yi] * (xp - xi) * (1.0 - yp + yi) +
        img[xi + xdim * (yi + 1)] * (1.0 - xp + xi) * (yp - yi) +
        img[1 + xi + xdim * (yi + 1)] * (xp - xi) * (yp - yi);

      xi = (int)xn;
      yi = (int)yn;
      valn = img[xi + xdim * yi] * (1.0 - xn + xi) * (1.0 - yn + yi) +
        img[1 + xi + xdim * yi] * (xn - xi) * (1.0 - yn + yi) +
        img[xi + xdim * (yi + 1)] * (1.0 - xn + xi) * (yn - yi) +
        img[1 + xi + xdim * (yi + 1)] * (xn - xi) * (yn - yi);

      result[0] = x;
      result[1] = y;
      result[2] = z;

      // now fit to a parabola and find max
      c = mag;
      b = (valp - valn) / 2.0;
      a = (valp - c - b);
      // assign the root to c because MSVC5.0 optimizer has problems with this
      // function
      c = -0.5 * b / a;
      if (c > 1.0)
      {
        c = 1.0;
      }
      if (c < -1.0)
      {
        c = -1.0;
      }
      result[0] += vec[0] * c;
      result[1] += vec[1] * c;

      // now calc the normal, trilinear interp of vectors
      xi = (int)result[0];
      yi = (int)result[1];
      // zi = (int)result[2];
      xn = result[0];
      yn = result[1];
      // zn = result[2];
      for (i = 0; i < 3; i++)
      {
        resultNormal[i] = inVecs->GetTuple(xi + xdim * yi)[i] * (1.0 - xn + xi) * (1.0 - yn + yi) +
          inVecs->GetTuple(1 + xi + xdim * yi)[i] * (xn - xi) * (1.0 - yn + yi) +
          inVecs->GetTuple(xi + xdim * (yi + 1))[i] * (1.0 - xn + xi) * (yn - yi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + 1))[i] * (xn - xi) * (yn - yi);
      }
      svtkMath::Normalize(resultNormal);
    }
  }
  else
  {
    if (x < 1 || y < 1 || z < 1 || x >= (xdim - 2) || y >= (ydim - 2) || z >= (zdim - 2))
    {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      if (x < 0 || y < 0 || z < 0 || x > xdim || y > ydim || z > zdim)
      {
        resultNormal[0] = 1;
        resultNormal[1] = 1;
        resultNormal[2] = 1;
      }
      else
      {
        for (i = 0; i < 3; i++)
        {
          resultNormal[i] = inVecs->GetTuple(x + xdim * y + xdim * ydim * z)[i];
        }
        svtkMath::Normalize(resultNormal);
      }
    }
    else
    {
      // first get the orientation
      inVecs->GetTuple(x + ypos + zpos, vec);
      vec[0] = vec[0] * spacing[0];
      vec[1] = vec[1] * spacing[1];
      vec[2] = vec[2] * spacing[2];
      svtkMath::Normalize(vec);
      mag = img[x + ypos + zpos];

      // compute the sample points
      xp = (double)x + vec[0];
      yp = (double)y + vec[1];
      zp = (double)z + vec[2];
      xn = (double)x - vec[0];
      yn = (double)y - vec[1];
      zn = (double)z - vec[2];

      // compute their values
      xi = (int)xp;
      yi = (int)yp;
      zi = (int)zp;

      // This set of statements used to be one statement. It was broken up
      // due to problems with MSVC 5.0
      valp =
        img[xi + xdim * (yi + zi * ydim)] * (1.0 - xp + xi) * (1.0 - yp + yi) * (1.0 - zp + zi);
      valp += img[1 + xi + xdim * (yi + zi * ydim)] * (xp - xi) * (1.0 - yp + yi) * (1.0 - zp + zi);
      valp += img[xi + xdim * (yi + 1 + zi * ydim)] * (1.0 - xp + xi) * (yp - yi) * (1.0 - zp + zi);
      valp += img[1 + xi + xdim * (yi + 1 + zi * ydim)] * (xp - xi) * (yp - yi) * (1.0 - zp + zi);
      valp +=
        img[xi + xdim * (yi + (zi + 1) * ydim)] * (1.0 - xp + xi) * (1.0 - yp + yi) * (zp - zi);
      valp += img[1 + xi + xdim * (yi + (zi + 1) * ydim)] * (xp - xi) * (1.0 - yp + yi) * (zp - zi);
      valp += img[xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (1.0 - xp + xi) * (yp - yi) * (zp - zi);
      valp += img[1 + xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (xp - xi) * (yp - yi) * (zp - zi);

      xi = (int)xn;
      yi = (int)yn;
      zi = (int)zn;
      // This set of statements used to be one statement. It was broken up
      // due to problems with MSVC 5.0
      valn =
        img[xi + xdim * (yi + zi * ydim)] * (1.0 - xn + xi) * (1.0 - yn + yi) * (1.0 - zn + zi);
      valn += img[1 + xi + xdim * (yi + zi * ydim)] * (xn - xi) * (1.0 - yn + yi) * (1.0 - zn + zi);
      valn += img[xi + xdim * (yi + 1 + zi * ydim)] * (1.0 - xn + xi) * (yn - yi) * (1.0 - zn + zi);
      valn += img[1 + xi + xdim * (yi + 1 + zi * ydim)] * (xn - xi) * (yn - yi) * (1.0 - zn + zi);
      valn +=
        img[xi + xdim * (yi + (zi + 1) * ydim)] * (1.0 - xn + xi) * (1.0 - yn + yi) * (zn - zi);
      valn += img[1 + xi + xdim * (yi + (zi + 1) * ydim)] * (xn - xi) * (1.0 - yn + yi) * (zn - zi);
      valn += img[xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (1.0 - xn + xi) * (yn - yi) * (zn - zi);
      valn += img[1 + xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (xn - xi) * (yn - yi) * (zn - zi);

      result[0] = x;
      result[1] = y;
      result[2] = z;

      if (this->TargetFlag)
      {
        // For target, do a simple linear interpolation to avoid binomial.
        c = mag;
        if (c == this->TargetValue)
        {
          c = 0.0;
        }
        else if ((this->TargetValue < c && valp < c) || (this->TargetValue > c && valp > c))
        {
          c = (this->TargetValue - c) / (valp - c);
        }
        else if ((this->TargetValue < c && valn < c) || (this->TargetValue < c && valn > c))
        {
          c = (this->TargetValue - c) / (c - valn);
        }
        else
        {
          c = 0.0;
        }
      }
      else
      {
        // now fit to a parabola and find max
        c = mag;
        b = (valp - valn) / 2.0;
        a = (valp - c - b);

        // assign the root to c because MSVC5.0 optimizer has problems with this
        // function
        c = -0.5 * b / a;
      }

      if (c > 1.0)
      {
        c = 1.0;
      }
      if (c < -1.0)
      {
        c = -1.0;
      }
      result[0] = result[0] + vec[0] * c;
      result[1] = result[1] + vec[1] * c;
      result[2] = result[2] + vec[2] * c;

      // now calc the normal, trilinear interp of vectors
      xi = (int)result[0];
      yi = (int)result[1];
      zi = (int)result[2];
      xn = result[0];
      yn = result[1];
      zn = result[2];

      for (i = 0; i < 3; i++)
      {
        resultNormal[i] = inVecs->GetTuple(xi + xdim * (yi + zi * ydim))[i] * (1.0 - xn + xi) *
            (1.0 - yn + yi) * (1.0 - zn + zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + zi * ydim))[i] * (xn - xi) * (1.0 - yn + yi) *
            (1.0 - zn + zi) +
          inVecs->GetTuple(xi + xdim * (yi + 1 + zi * ydim))[i] * (1.0 - xn + xi) * (yn - yi) *
            (1.0 - zn + zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + 1 + zi * ydim))[i] * (xn - xi) * (yn - yi) *
            (1.0 - zn + zi) +
          inVecs->GetTuple(xi + xdim * (yi + (zi + 1) * ydim))[i] * (1.0 - xn + xi) *
            (1.0 - yn + yi) * (zn - zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + (zi + 1) * ydim))[i] * (xn - xi) *
            (1.0 - yn + yi) * (zn - zi) +
          inVecs->GetTuple(xi + xdim * (yi + 1 + (zi + 1) * ydim))[i] * (1.0 - xn + xi) *
            (yn - yi) * (zn - zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + 1 + (zi + 1) * ydim))[i] * (xn - xi) * (yn - yi) *
            (zn - zi);
      }
      svtkMath::Normalize(resultNormal);
    }
  }
}

void svtkSubPixelPositionEdgels::Move(int xdim, int ydim, int zdim, int x, int y, float* img,
  svtkDataArray* inVecs, double* result, int z, double* spacing, double* resultNormal)
{
  int ypos;
  svtkIdType zpos;
  double vec[3];
  double valn, valp;
  double mag;
  double a, b, c;
  double xp, yp, zp;
  double xn, yn, zn;
  int xi, yi, zi, i;

  zpos = z * xdim * ydim;

  ypos = y * xdim;

  // handle the 2d case
  if (zdim < 2)
  {
    if (x < 1 || y < 1 || x >= (xdim - 2) || y >= (ydim - 2))
    {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      // if the point of off of the grad map just make up a value
      if (x < 0 || y < 0 || x > xdim || y > ydim)
      {
        resultNormal[0] = 1;
        resultNormal[1] = 0;
        resultNormal[2] = 0;
      }
      else
      {
        for (i = 0; i < 3; i++)
        {
          resultNormal[i] = inVecs->GetTuple(x + xdim * y)[i];
        }
        svtkMath::Normalize(resultNormal);
      }
    }
    else
    {
      // first get the orientation
      inVecs->GetTuple(x + ypos, vec);
      vec[0] = vec[0] * spacing[0];
      vec[1] = vec[1] * spacing[1];
      vec[2] = 0;
      svtkMath::Normalize(vec);
      mag = img[x + ypos];

      // compute the sample points
      xp = (double)x + vec[0];
      yp = (double)y + vec[1];
      xn = (double)x - vec[0];
      yn = (double)y - vec[1];

      // compute their values
      xi = (int)xp;
      yi = (int)yp;
      valp = img[xi + xdim * yi] * (1.0 - xp + xi) * (1.0 - yp + yi) +
        img[1 + xi + xdim * yi] * (xp - xi) * (1.0 - yp + yi) +
        img[xi + xdim * (yi + 1)] * (1.0 - xp + xi) * (yp - yi) +
        img[1 + xi + xdim * (yi + 1)] * (xp - xi) * (yp - yi);

      xi = (int)xn;
      yi = (int)yn;
      valn = img[xi + xdim * yi] * (1.0 - xn + xi) * (1.0 - yn + yi) +
        img[1 + xi + xdim * yi] * (xn - xi) * (1.0 - yn + yi) +
        img[xi + xdim * (yi + 1)] * (1.0 - xn + xi) * (yn - yi) +
        img[1 + xi + xdim * (yi + 1)] * (xn - xi) * (yn - yi);

      result[0] = x;
      result[1] = y;
      result[2] = z;

      // now fit to a parabola and find max
      c = mag;
      b = (valp - valn) / 2.0;
      a = (valp - c - b);
      // assign the root to c because MSVC5.0 optimizer has problems with this
      // function
      c = -0.5 * b / a;
      if (c > 1.0)
      {
        c = 1.0;
      }
      if (c < -1.0)
      {
        c = -1.0;
      }
      result[0] += vec[0] * c;
      result[1] += vec[1] * c;

      // now calc the normal, trilinear interp of vectors
      xi = (int)result[0];
      yi = (int)result[1];
      // zi = (int)result[2];
      xn = result[0];
      yn = result[1];
      // zn = result[2];
      for (i = 0; i < 3; i++)
      {
        resultNormal[i] = inVecs->GetTuple(xi + xdim * yi)[i] * (1.0 - xn + xi) * (1.0 - yn + yi) +
          inVecs->GetTuple(1 + xi + xdim * yi)[i] * (xn - xi) * (1.0 - yn + yi) +
          inVecs->GetTuple(xi + xdim * (yi + 1))[i] * (1.0 - xn + xi) * (yn - yi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + 1))[i] * (xn - xi) * (yn - yi);
      }
      svtkMath::Normalize(resultNormal);
    }
  }
  else
  {
    if (x < 1 || y < 1 || z < 1 || x >= (xdim - 2) || y >= (ydim - 2) || z >= (zdim - 2))
    {
      result[0] = x;
      result[1] = y;
      result[2] = z;
      if (x < 0 || y < 0 || z < 0 || x > xdim || y > ydim || z > zdim)
      {
        resultNormal[0] = 1;
        resultNormal[1] = 1;
        resultNormal[2] = 1;
      }
      else
      {
        for (i = 0; i < 3; i++)
        {
          resultNormal[i] = inVecs->GetTuple(x + xdim * y + xdim * ydim * z)[i];
        }
        svtkMath::Normalize(resultNormal);
      }
    }
    else
    {
      // first get the orientation
      inVecs->GetTuple(x + ypos + zpos, vec);
      vec[0] = vec[0] * spacing[0];
      vec[1] = vec[1] * spacing[1];
      vec[2] = vec[2] * spacing[2];
      svtkMath::Normalize(vec);
      mag = img[x + ypos + zpos];

      // compute the sample points
      xp = (double)x + vec[0];
      yp = (double)y + vec[1];
      zp = (double)z + vec[2];
      xn = (double)x - vec[0];
      yn = (double)y - vec[1];
      zn = (double)z - vec[2];

      // compute their values
      xi = (int)xp;
      yi = (int)yp;
      zi = (int)zp;

      // This set of statements used to be one statement. It was broken up
      // due to problems with MSVC 5.0
      valp =
        img[xi + xdim * (yi + zi * ydim)] * (1.0 - xp + xi) * (1.0 - yp + yi) * (1.0 - zp + zi);
      valp += img[1 + xi + xdim * (yi + zi * ydim)] * (xp - xi) * (1.0 - yp + yi) * (1.0 - zp + zi);
      valp += img[xi + xdim * (yi + 1 + zi * ydim)] * (1.0 - xp + xi) * (yp - yi) * (1.0 - zp + zi);
      valp += img[1 + xi + xdim * (yi + 1 + zi * ydim)] * (xp - xi) * (yp - yi) * (1.0 - zp + zi);
      valp +=
        img[xi + xdim * (yi + (zi + 1) * ydim)] * (1.0 - xp + xi) * (1.0 - yp + yi) * (zp - zi);
      valp += img[1 + xi + xdim * (yi + (zi + 1) * ydim)] * (xp - xi) * (1.0 - yp + yi) * (zp - zi);
      valp += img[xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (1.0 - xp + xi) * (yp - yi) * (zp - zi);
      valp += img[1 + xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (xp - xi) * (yp - yi) * (zp - zi);

      xi = (int)xn;
      yi = (int)yn;
      zi = (int)zn;
      // This set of statements used to be one statement. It was broken up
      // due to problems with MSVC 5.0
      valn =
        img[xi + xdim * (yi + zi * ydim)] * (1.0 - xn + xi) * (1.0 - yn + yi) * (1.0 - zn + zi);
      valn += img[1 + xi + xdim * (yi + zi * ydim)] * (xn - xi) * (1.0 - yn + yi) * (1.0 - zn + zi);
      valn += img[xi + xdim * (yi + 1 + zi * ydim)] * (1.0 - xn + xi) * (yn - yi) * (1.0 - zn + zi);
      valn += img[1 + xi + xdim * (yi + 1 + zi * ydim)] * (xn - xi) * (yn - yi) * (1.0 - zn + zi);
      valn +=
        img[xi + xdim * (yi + (zi + 1) * ydim)] * (1.0 - xn + xi) * (1.0 - yn + yi) * (zn - zi);
      valn += img[1 + xi + xdim * (yi + (zi + 1) * ydim)] * (xn - xi) * (1.0 - yn + yi) * (zn - zi);
      valn += img[xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (1.0 - xn + xi) * (yn - yi) * (zn - zi);
      valn += img[1 + xi + xdim * (yi + 1 + (zi + 1) * ydim)] * (xn - xi) * (yn - yi) * (zn - zi);

      result[0] = x;
      result[1] = y;
      result[2] = z;

      if (this->TargetFlag)
      {
        // For target, do a simple linear interpolation to avoid binomial.
        c = mag;
        if (c == this->TargetValue)
        {
          c = 0.0;
        }
        else if ((this->TargetValue < c && valp < c) || (this->TargetValue > c && valp > c))
        {
          c = (this->TargetValue - c) / (valp - c);
        }
        else if ((this->TargetValue < c && valn < c) || (this->TargetValue < c && valn > c))
        {
          c = (this->TargetValue - c) / (c - valn);
        }
        else
        {
          c = 0.0;
        }
      }
      else
      {
        // now fit to a parabola and find max
        c = mag;
        b = (valp - valn) / 2.0;
        a = (valp - c - b);

        // assign the root to c because MSVC5.0 optimizer has problems with this
        // function
        c = -0.5 * b / a;
      }

      if (c > 1.0)
      {
        c = 1.0;
      }
      if (c < -1.0)
      {
        c = -1.0;
      }
      result[0] = result[0] + vec[0] * c;
      result[1] = result[1] + vec[1] * c;
      result[2] = result[2] + vec[2] * c;

      // now calc the normal, trilinear interp of vectors
      xi = (int)result[0];
      yi = (int)result[1];
      zi = (int)result[2];
      xn = result[0];
      yn = result[1];
      zn = result[2];

      for (i = 0; i < 3; i++)
      {
        resultNormal[i] = inVecs->GetTuple(xi + xdim * (yi + zi * ydim))[i] * (1.0 - xn + xi) *
            (1.0 - yn + yi) * (1.0 - zn + zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + zi * ydim))[i] * (xn - xi) * (1.0 - yn + yi) *
            (1.0 - zn + zi) +
          inVecs->GetTuple(xi + xdim * (yi + 1 + zi * ydim))[i] * (1.0 - xn + xi) * (yn - yi) *
            (1.0 - zn + zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + 1 + zi * ydim))[i] * (xn - xi) * (yn - yi) *
            (1.0 - zn + zi) +
          inVecs->GetTuple(xi + xdim * (yi + (zi + 1) * ydim))[i] * (1.0 - xn + xi) *
            (1.0 - yn + yi) * (zn - zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + (zi + 1) * ydim))[i] * (xn - xi) *
            (1.0 - yn + yi) * (zn - zi) +
          inVecs->GetTuple(xi + xdim * (yi + 1 + (zi + 1) * ydim))[i] * (1.0 - xn + xi) *
            (yn - yi) * (zn - zi) +
          inVecs->GetTuple(1 + xi + xdim * (yi + 1 + (zi + 1) * ydim))[i] * (xn - xi) * (yn - yi) *
            (zn - zi);
      }
      svtkMath::Normalize(resultNormal);
    }
  }
}

void svtkSubPixelPositionEdgels::SetGradMapsData(svtkStructuredPoints* gm)
{
  this->SetInputData(1, gm);
}

svtkStructuredPoints* svtkSubPixelPositionEdgels::GetGradMaps()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkStructuredPoints::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

int svtkSubPixelPositionEdgels::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredPoints");
    return 1;
  }

  return this->Superclass::FillInputPortInformation(port, info);
}

// Print the state of the class.
void svtkSubPixelPositionEdgels::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->GetGradMaps())
  {
    os << indent << "Gradient Data: " << this->GetGradMaps() << "\n";
  }
  else
  {
    os << indent << "Gradient Data: (none)\n";
  }

  os << indent << "TargetFlag: " << this->TargetFlag << endl;
  os << indent << "TargetValue: " << this->TargetValue << endl;
}
