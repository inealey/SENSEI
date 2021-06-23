/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTriangleMeshPointNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkActor.h"
#include "svtkArrowSource.h"
#include "svtkCamera.h"
#include "svtkCleanPolyData.h"
#include "svtkGlyph3D.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTriangleFilter.h"
#include "svtkTriangleMeshPointNormals.h"
#include "svtkXMLPolyDataReader.h"

int TestTriangleMeshPointNormals(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  std::cout << fileName << std::endl;

  // reader
  svtkNew<svtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  // triangle filter
  svtkNew<svtkTriangleFilter> triFilter;
  triFilter->SetInputConnection(reader->GetOutputPort());

  // cleaning filter
  svtkNew<svtkCleanPolyData> cleanFilter;
  cleanFilter->SetInputConnection(triFilter->GetOutputPort());

  // normals
  svtkNew<svtkTriangleMeshPointNormals> normFilter;
  normFilter->SetInputConnection(cleanFilter->GetOutputPort());

  // mapper, actor
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(normFilter->GetOutputPort());
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  // glyphs
  svtkNew<svtkArrowSource> glyphsource;
  svtkNew<svtkGlyph3D> glyph;
  glyph->SetInputConnection(normFilter->GetOutputPort());
  glyph->SetSourceConnection(glyphsource->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetColorModeToColorByVector();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.5);
  svtkNew<svtkPolyDataMapper> glyphmapper;
  glyphmapper->SetInputConnection(glyph->GetOutputPort());
  svtkNew<svtkActor> glyphactor;
  glyphactor->SetMapper(glyphmapper);

  // renderer
  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(glyphactor);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->ResetCamera();

  // renderwindow, interactor
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  iren->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    svtkNew<svtkInteractorStyleTrackballCamera> iStyle;
    iren->SetInteractorStyle(iStyle);
    renWin->SetSize(1000, 1000);
    iren->Start();
  }

  return !retVal;
}
