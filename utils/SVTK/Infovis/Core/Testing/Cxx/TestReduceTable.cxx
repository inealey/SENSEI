/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestReduceTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkReduceTable.h"

#include "svtkDoubleArray.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

//----------------------------------------------------------------------------
int TestReduceTable(int, char*[])
{
  svtkNew<svtkTable> table;

  svtkNew<svtkStringArray> indexColumn;
  indexColumn->SetNumberOfTuples(6);
  indexColumn->SetValue(0, "a");
  indexColumn->SetValue(1, "b");
  indexColumn->SetValue(2, "b");
  indexColumn->SetValue(3, "c");
  indexColumn->SetValue(4, "c");
  indexColumn->SetValue(5, "c");

  svtkNew<svtkDoubleArray> meanColumn;
  meanColumn->SetNumberOfTuples(6);
  meanColumn->SetValue(0, 1.0);
  meanColumn->SetValue(1, 1.0);
  meanColumn->SetValue(2, 3.0);
  meanColumn->SetValue(3, 1.0);
  meanColumn->SetValue(4, 3.0);
  meanColumn->SetValue(5, 5.0);

  svtkNew<svtkIntArray> medianColumn;
  medianColumn->SetNumberOfTuples(6);
  medianColumn->SetValue(0, 2);
  medianColumn->SetValue(1, 3);
  medianColumn->SetValue(2, 5);
  medianColumn->SetValue(3, 4);
  medianColumn->SetValue(4, 6);
  medianColumn->SetValue(5, 20);

  svtkNew<svtkStringArray> modeColumn;
  modeColumn->SetNumberOfTuples(6);
  modeColumn->SetValue(0, "a");
  modeColumn->SetValue(1, "b");
  modeColumn->SetValue(2, "b");
  modeColumn->SetValue(3, "c");
  modeColumn->SetValue(4, "c");
  modeColumn->SetValue(5, "d");

  table->AddColumn(indexColumn);
  table->AddColumn(meanColumn);
  table->AddColumn(medianColumn);
  table->AddColumn(modeColumn);

  svtkNew<svtkReduceTable> filter;
  filter->SetInputData(0, table);
  filter->SetIndexColumn(0);
  filter->SetReductionMethodForColumn(1, svtkReduceTable::MEAN);
  filter->SetReductionMethodForColumn(2, svtkReduceTable::MEDIAN);
  filter->SetReductionMethodForColumn(3, svtkReduceTable::MODE);
  filter->Update();

  svtkTable* output = filter->GetOutput();

  if (output->GetValue(0, 1) != 1)
  {
    cout << "ERROR: incorrect value encountered at (0, 1)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(1, 1) != 2)
  {
    cout << "ERROR: incorrect value encountered at (1, 1)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(2, 1) != 3)
  {
    cout << "ERROR: incorrect value encountered at (2, 1)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(0, 2) != 2)
  {
    cout << "ERROR: incorrect value encountered at (0, 2)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(1, 2) != 4)
  {
    cout << "ERROR: incorrect value encountered at (1, 2)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(2, 2) != 6)
  {
    cout << "ERROR: incorrect value encountered at (2, 2)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(0, 3) != "a")
  {
    cout << "ERROR: incorrect value encountered at (0, 3)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(1, 3) != "b")
  {
    cout << "ERROR: incorrect value encountered at (1, 3)" << endl;
    return EXIT_FAILURE;
  }
  if (output->GetValue(2, 3) != "c")
  {
    cout << "ERROR: incorrect value encountered at (2, 3)" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
