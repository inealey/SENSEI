/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRectangularButtonSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMinimalStandardRandomSequence.h>
#include <svtkRectangularButtonSource.h>
#include <svtkSmartPointer.h>

int TestRectangularButtonSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkRectangularButtonSource> rectangularButtonSource =
    svtkSmartPointer<svtkRectangularButtonSource>::New();
  rectangularButtonSource->SetBoxRatio(1.0);
  rectangularButtonSource->SetTextureHeightRatio(1.0);
  rectangularButtonSource->SetTextureRatio(1.0);
  rectangularButtonSource->SetShoulderTextureCoordinate(0.0, 0.0);
  rectangularButtonSource->SetTextureDimensions(100, 100);
  rectangularButtonSource->SetTextureStyleToProportional();
  rectangularButtonSource->TwoSidedOff();

  rectangularButtonSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  rectangularButtonSource->SetCenter(center);

  randomSequence->Next();
  double depth = randomSequence->GetValue();
  rectangularButtonSource->SetDepth(depth);

  randomSequence->Next();
  double height = randomSequence->GetValue();
  rectangularButtonSource->SetHeight(height);

  randomSequence->Next();
  double width = randomSequence->GetValue();
  rectangularButtonSource->SetWidth(width);

  rectangularButtonSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = rectangularButtonSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  rectangularButtonSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  rectangularButtonSource->SetCenter(center);

  randomSequence->Next();
  depth = randomSequence->GetValue();
  rectangularButtonSource->SetDepth(depth);

  randomSequence->Next();
  height = randomSequence->GetValue();
  rectangularButtonSource->SetHeight(height);

  randomSequence->Next();
  width = randomSequence->GetValue();
  rectangularButtonSource->SetWidth(width);

  rectangularButtonSource->Update();

  polyData = rectangularButtonSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
