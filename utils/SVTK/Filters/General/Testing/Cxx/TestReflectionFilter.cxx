/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestReflectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests svtkReflectionFilter

#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkReflectionFilter.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

#include <cassert>
#include <iostream>

#define AssertMacro(b)                                                                             \
  if (!(b))                                                                                        \
  {                                                                                                \
    std::cerr << "Failed to reflect pyramid" << std::endl;                                         \
    return EXIT_FAILURE;                                                                           \
  }

int TestReflectionFilter(int, char*[])
{
  svtkSmartPointer<svtkUnstructuredGrid> pyramid = svtkSmartPointer<svtkUnstructuredGrid>::New();
  {
    svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
    points->InsertNextPoint(-1, -1, -1);
    points->InsertNextPoint(1, -1, -1);
    points->InsertNextPoint(1, 1, -1);
    points->InsertNextPoint(-1, 1, -1);
    points->InsertNextPoint(0, 0, 1);
    pyramid->SetPoints(points);

    svtkPointData* pd = pyramid->GetPointData();
    svtkNew<svtkDoubleArray> array;
    array->SetNumberOfComponents(3);
    double tuple[3] = { 1, 1, 13 };
    array->InsertNextTuple(tuple);
    array->InsertNextTuple(tuple);
    array->InsertNextTuple(tuple);
    array->InsertNextTuple(tuple);
    pd->AddArray(array);

    svtkNew<svtkDoubleArray> tensor;
    tensor->SetNumberOfComponents(9);
    double tensorTuple[9] = { 1, 1, 7, 1, 1, 1, 1, 1, 1 };
    tensor->InsertNextTuple(tensorTuple);
    tensor->InsertNextTuple(tensorTuple);
    tensor->InsertNextTuple(tensorTuple);
    tensor->InsertNextTuple(tensorTuple);
    pd->SetTensors(tensor);

    svtkCellData* cd = pyramid->GetCellData();
    svtkNew<svtkDoubleArray> symTensor;
    symTensor->SetNumberOfComponents(6);
    double symTensorTuple[6] = { 1, 1, 1, 1, 17, 1 };
    symTensor->InsertNextTuple(symTensorTuple);
    symTensor->InsertNextTuple(symTensorTuple);
    symTensor->InsertNextTuple(symTensorTuple);
    symTensor->InsertNextTuple(symTensorTuple);
    cd->AddArray(symTensor);
  }

  svtkNew<svtkIdList> verts;
  verts->InsertNextId(0);
  verts->InsertNextId(1);
  verts->InsertNextId(2);
  verts->InsertNextId(3);
  verts->InsertNextId(4);

  pyramid->InsertNextCell(SVTK_PYRAMID, verts.GetPointer());

  for (int i = 0; i < 2; i++)
  {
    svtkSmartPointer<svtkReflectionFilter> reflectionFilter =
      svtkSmartPointer<svtkReflectionFilter>::New();
    reflectionFilter->SetInputData(pyramid.GetPointer());
    if (i == 0)
    {
      reflectionFilter->CopyInputOff();
      reflectionFilter->FlipAllInputArraysOff();
    }
    else
    {
      reflectionFilter->CopyInputOn();
      reflectionFilter->FlipAllInputArraysOn();
    }
    reflectionFilter->SetPlaneToZMin();
    reflectionFilter->Update();
    svtkUnstructuredGrid* pyramid1 =
      svtkUnstructuredGrid::SafeDownCast(reflectionFilter->GetOutput());
    svtkNew<svtkIdList> cellIds;
    if (i == 0)
    {
      AssertMacro(pyramid1->GetNumberOfCells() == 1);
      AssertMacro(pyramid1->GetPointData()->GetTensors()->GetComponent(0, 2) == -7);
    }
    else
    {
      AssertMacro(pyramid1->GetNumberOfCells() == 2);
      AssertMacro(pyramid1->GetPointData()->GetArray(0)->GetComponent(5, 2) == -13);
      AssertMacro(pyramid1->GetCellData()->GetArray(0)->GetComponent(1, 4) == -17);
      AssertMacro(pyramid1->GetPointData()->GetTensors()->GetComponent(5, 2) == -7);
    }
    pyramid1->GetCellPoints(i, cellIds.GetPointer());
    int apex = cellIds->GetId(4);
    int offset = i == 0 ? 0 : 5;
    AssertMacro(apex == 4 + offset);
    for (int j = 0; j < 4; j++)
    {
      int next = cellIds->GetId((j + 1) % 4);
      int nextExpected = (cellIds->GetId(j) - offset + 3) % 4 + offset;
      AssertMacro(next == nextExpected);
    }
  }

  return EXIT_SUCCESS;
}
