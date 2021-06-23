/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PlotSPHKernels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Plot the SPH kernel functions and derivatives.

#include "svtkSPHCubicKernel.h"
#include "svtkSPHQuarticKernel.h"
#include "svtkSPHQuinticKernel.h"
#include "svtkSmartPointer.h"
#include "svtkWendlandQuinticKernel.h"

#include <svtkAxis.h>
#include <svtkChartXY.h>
#include <svtkContextScene.h>
#include <svtkContextView.h>
#include <svtkFloatArray.h>
#include <svtkPen.h>
#include <svtkPlot.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTable.h>

//-----------------------------------------------------------------------------
// Helper function
template <class T>
void AddKernelToPlot(svtkSmartPointer<T> kernel, svtkSmartPointer<svtkChartXY> chart,
  const std::string& description, svtkSmartPointer<svtkTable> table, unsigned char rgb[3])
{
  double r, width = 3.5;
  int res = 100;
  double inc = width / static_cast<double>(res);
  double fVal, dVal;

  int numCols = table->GetNumberOfColumns();
  numCols = (numCols < 1 ? 1 : numCols);

  svtkSmartPointer<svtkFloatArray> arrX = svtkSmartPointer<svtkFloatArray>::New();
  arrX->SetName("X Axis");
  arrX->SetNumberOfValues(res);
  table->AddColumn(arrX);

  svtkSmartPointer<svtkFloatArray> arrC = svtkSmartPointer<svtkFloatArray>::New();
  arrC->SetName(description.c_str());
  arrC->SetNumberOfValues(res);
  table->AddColumn(arrC);

  std::string deriv = description + "_deriv";
  svtkSmartPointer<svtkFloatArray> arrS = svtkSmartPointer<svtkFloatArray>::New();
  arrS->SetName(deriv.c_str());
  arrS->SetNumberOfValues(res);
  table->AddColumn(arrS);

  // Fill in the table with function values
  table->SetNumberOfRows(res);
  for (int i = 0; i < res; ++i)
  {
    r = static_cast<double>(i) * inc;
    fVal = kernel->GetNormFactor() * kernel->ComputeFunctionWeight(r);
    dVal = kernel->GetNormFactor() * kernel->ComputeDerivWeight(r);

    table->SetValue(i, 0, r);
    table->SetValue(i, numCols, fVal);
    table->SetValue(i, numCols + 1, dVal);
  }

  svtkPlot* line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, numCols);
  line->SetColor(rgb[0], rgb[1], rgb[2], 255);
  line->SetWidth(1.0);

  line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, numCols + 1);
  line->SetColor(rgb[0], rgb[1], rgb[2], 255);
  line->SetWidth(1.0);
}

//-----------------------------------------------------------------------------
int PlotSPHKernels(int, char*[])
{
  // Set up the view
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderWindow()->SetSize(400, 300);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);

  // Add multiple line plots, setting the colors etc
  svtkSmartPointer<svtkChartXY> chart = svtkSmartPointer<svtkChartXY>::New();
  chart->SetTitle("SPH Kernels");
  chart->SetShowLegend(true);
  view->GetScene()->AddItem(chart);

  // Create a table which will contain the plots
  svtkSmartPointer<svtkTable> table = svtkSmartPointer<svtkTable>::New();

  unsigned char rgb[3];
  // We will approach the kernel from beyond the cutoff distance and plot the
  // function and derivative values.

  // Cubic SPH Kernel
  svtkSmartPointer<svtkSPHCubicKernel> cubic = svtkSmartPointer<svtkSPHCubicKernel>::New();
  cubic->SetDimension(2);
  cubic->SetSpatialStep(1.0);
  cubic->Initialize(nullptr, nullptr, nullptr);
  rgb[0] = 255;
  rgb[1] = 0;
  rgb[2] = 0;
  AddKernelToPlot<svtkSPHCubicKernel>(cubic, chart, "Cubic", table, rgb);

  // Quartic Kernel
  svtkSmartPointer<svtkSPHQuarticKernel> quartic = svtkSmartPointer<svtkSPHQuarticKernel>::New();
  quartic->SetDimension(2);
  quartic->SetSpatialStep(1.0);
  quartic->Initialize(nullptr, nullptr, nullptr);
  rgb[0] = 0;
  rgb[1] = 255;
  rgb[2] = 0;
  AddKernelToPlot<svtkSPHQuarticKernel>(quartic, chart, "Quartic", table, rgb);

  // Quintic Kernel
  svtkSmartPointer<svtkSPHQuinticKernel> quintic = svtkSmartPointer<svtkSPHQuinticKernel>::New();
  quintic->SetDimension(2);
  quintic->SetSpatialStep(1.0);
  quintic->Initialize(nullptr, nullptr, nullptr);
  rgb[0] = 0;
  rgb[1] = 0;
  rgb[2] = 255;
  AddKernelToPlot<svtkSPHQuinticKernel>(quintic, chart, "Quintic", table, rgb);

  // Wendland C2 (quintic) Kernel
  svtkSmartPointer<svtkWendlandQuinticKernel> wendland =
    svtkSmartPointer<svtkWendlandQuinticKernel>::New();
  wendland->SetDimension(2);
  wendland->SetSpatialStep(1.0);
  wendland->Initialize(nullptr, nullptr, nullptr);
  rgb[0] = 255;
  rgb[1] = 0;
  rgb[2] = 255;
  AddKernelToPlot<svtkWendlandQuinticKernel>(wendland, chart, "Wendland", table, rgb);

  svtkAxis* left = chart->GetAxis(svtkAxis::LEFT);
  svtkAxis* bottom = chart->GetAxis(svtkAxis::BOTTOM);
  left->SetTitle("Kernel Value");
  bottom->SetTitle("r/h");

  // Start interactor
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
