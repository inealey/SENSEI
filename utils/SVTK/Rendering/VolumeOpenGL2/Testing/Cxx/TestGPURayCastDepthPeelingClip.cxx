/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastDepthPeelingClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 *  Tests depth peeling pass with volume rendering + clipping.
 */

#include <svtkCallbackCommand.h>
#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOpenGLRenderer.h>
#include <svtkOutlineFilter.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkPlaneCollection.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderTimerLog.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkTestUtilities.h>
#include <svtkTestingObjectFactory.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

#include <cassert>

namespace
{

void RenderComplete(svtkObject* obj, unsigned long, void*, void*)
{
  svtkRenderWindow* renWin = svtkRenderWindow::SafeDownCast(obj);
  assert(renWin);

  svtkRenderTimerLog* timer = renWin->GetRenderTimer();
  while (timer->FrameReady())
  {
    std::cout << "-- Frame Timing:------------------------------------------\n";
    timer->PopFirstReadyFrame().Print(std::cout);
    std::cout << "\n";
  }
}

} // end anon namespace

class SamplingDistanceCallback : public svtkCommand
{
public:
  static SamplingDistanceCallback* New() { return new SamplingDistanceCallback; }

  void Execute(svtkObject* svtkNotUsed(caller), unsigned long event, void* svtkNotUsed(data)) override
  {
    switch (event)
    {
      case svtkCommand::StartInteractionEvent:
      {
        // Higher ImageSampleDistance to make the volume-rendered image's
        // resolution visibly lower during interaction.
        this->Mapper->SetImageSampleDistance(6.5);
      }
      break;

      case svtkCommand::EndInteractionEvent:
      {
        // Default ImageSampleDistance
        this->Mapper->SetImageSampleDistance(1.0);
      }
    }
  }

  svtkGPUVolumeRayCastMapper* Mapper = nullptr;
};

int TestGPURayCastDepthPeelingClip(int argc, char* argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkRenderer> ren;
  renWin->Render(); // Create the context
  renWin->AddRenderer(ren);
  svtkOpenGLRenderer* oglRen = svtkOpenGLRenderer::SafeDownCast(ren);
  assert(oglRen); // This test should only be enabled for OGL2 backend.
  // This will print details about why depth peeling is unsupported:
  oglRen->SetDebug(1);
  bool supported = oglRen->IsDualDepthPeelingSupported();
  oglRen->SetDebug(0);
  if (!supported)
  {
    std::cerr << "Skipping test; volume peeling not supported.\n";
    return SVTK_SKIP_RETURN_CODE;
  }

  // Setup the rendertimer observer:
  svtkNew<svtkCallbackCommand> renderCompleteCB;
  renderCompleteCB->SetCallback(RenderComplete);
  renWin->GetRenderTimer()->LoggingEnabledOn();
  renWin->AddObserver(svtkCommand::EndEvent, renderCompleteCB);

  double scalarRange[2];

  svtkNew<svtkActor> outlineActor;
  svtkNew<svtkPolyDataMapper> outlineMapper;
  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  // Add outline filter
  svtkNew<svtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  // Test clipping now
  const double* bounds = reader->GetOutput()->GetBounds();
  svtkNew<svtkPlane> clipPlane1;
  clipPlane1->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.0, 0.0);
  clipPlane1->SetNormal(0.8, 0.0, 0.0);

  svtkNew<svtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.35 * (bounds[2] + bounds[3]), 0.0);
  clipPlane2->SetNormal(0.2, -0.2, 0.0);

  svtkNew<svtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  clipPlaneCollection->AddItem(clipPlane2);
  volumeMapper->SetClippingPlanes(clipPlaneCollection);

  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);
  ren->SetBackground(0.0, 0.0, 0.0);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.6, 0.6);

  svtkSmartPointer<svtkVolume> volume = svtkSmartPointer<svtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  int dims[3];
  double spacing[3], center[3], origin[3];
  reader->Update();
  svtkSmartPointer<svtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  // Add sphere 1
  center[0] = origin[0] + spacing[0] * dims[0] / 2.0;
  center[1] = origin[1] + spacing[1] * dims[1] / 2.0;
  center[2] = origin[2] + spacing[2] * dims[2] / 2.0;

  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetCenter(center);
  sphereSource->SetRadius(dims[1] / 3.0);
  svtkNew<svtkActor> sphereActor;
  svtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(0.5, 0.9, 0.7);
  sphereProperty->SetOpacity(0.3);
  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  // Add sphere 2
  center[0] += 15.0;
  center[1] += 15.0;
  center[2] += 15.0;

  svtkNew<svtkSphereSource> sphereSource2;
  sphereSource2->SetCenter(center);
  sphereSource2->SetRadius(dims[1] / 3.0);
  svtkNew<svtkActor> sphereActor2;
  sphereProperty = sphereActor2->GetProperty();
  sphereProperty->SetColor(0.9, 0.4, 0.1);
  sphereProperty->SetOpacity(0.3);
  svtkNew<svtkPolyDataMapper> sphereMapper2;
  sphereMapper2->SetInputConnection(sphereSource2->GetOutputPort());
  sphereActor2->SetMapper(sphereMapper2);

  // Add actors
  ren->AddVolume(volume);
  ren->AddActor(outlineActor);
  ren->AddActor(sphereActor);
  ren->AddActor(sphereActor2);

  // Configure depth peeling
  ren->SetUseDepthPeeling(1);
  ren->SetOcclusionRatio(0.0);
  ren->SetMaximumNumberOfPeels(17);
  ren->SetUseDepthPeelingForVolumes(true);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  renWin->GetInteractor()->SetInteractorStyle(style);

  svtkNew<SamplingDistanceCallback> callback;
  callback->Mapper = volumeMapper;
  style->AddObserver(svtkCommand::StartInteractionEvent, callback);
  style->AddObserver(svtkCommand::EndInteractionEvent, callback);

  ren->ResetCamera();
  renWin->Render();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
