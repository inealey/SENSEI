/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSphereWidgetZoomInOut.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCommand.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereWidget.h"
#include "svtkTestUtilities.h"

// Callback for the interaction
class svtkSphWCallback : public svtkCommand
{
public:
  static svtkSphWCallback* New() { return new svtkSphWCallback; }
  void Execute(svtkObject*, unsigned long, void*) override {}
  svtkSphWCallback() = default;
};

const char SphereWidgetEventLog[] = "# StreamVersion 1\n"
                                    "CharEvent 187 242 0 0 105 1 i\n"
                                    "KeyReleaseEvent 117 127 0 0 105 1 i\n"
                                    "RightButtonPressEvent 117 127 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 126 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 124 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 123 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 122 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 121 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 120 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 118 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 117 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 116 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 115 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 114 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 113 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 112 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 111 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 110 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 109 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 108 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 107 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 106 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 105 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 104 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 103 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 101 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 100 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 99 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 98 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 96 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 94 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 92 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 90 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 89 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 87 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 84 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 83 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 81 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 79 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 77 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 76 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 73 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 72 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 70 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 68 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 67 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 65 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 64 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 63 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 61 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 60 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 58 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 56 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 55 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 52 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 50 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 49 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 47 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 45 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 43 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 41 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 40 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 35 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 34 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 33 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 32 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 30 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 29 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 28 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 27 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 26 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 24 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 23 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 22 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 20 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 18 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 16 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 14 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 13 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 10 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 9 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 7 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 6 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 5 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 3 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 0 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -2 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -3 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -5 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -8 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -11 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -12 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -14 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -16 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -17 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -21 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -23 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -25 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -28 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -29 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -31 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -34 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -37 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -39 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -41 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -42 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -44 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -45 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -47 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -49 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -50 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -52 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -54 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -55 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -56 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -58 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -60 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -61 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -62 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -63 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -64 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -64 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -65 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -66 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -67 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -68 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -67 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -64 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -61 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -54 0 0 0 0 i\n"
                                    "MouseMoveEvent 117 -51 0 0 0 0 i\n"
                                    "MouseMoveEvent 116 -48 0 0 0 0 i\n"
                                    "MouseMoveEvent 115 -45 0 0 0 0 i\n"
                                    "MouseMoveEvent 114 -43 0 0 0 0 i\n"
                                    "MouseMoveEvent 114 -39 0 0 0 0 i\n"
                                    "MouseMoveEvent 113 -36 0 0 0 0 i\n"
                                    "MouseMoveEvent 113 -34 0 0 0 0 i\n"
                                    "MouseMoveEvent 112 -31 0 0 0 0 i\n"
                                    "MouseMoveEvent 112 -30 0 0 0 0 i\n"
                                    "MouseMoveEvent 112 -28 0 0 0 0 i\n"
                                    "MouseMoveEvent 111 -26 0 0 0 0 i\n"
                                    "MouseMoveEvent 111 -24 0 0 0 0 i\n"
                                    "MouseMoveEvent 111 -21 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -19 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -18 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -16 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -13 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -12 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -11 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -9 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -8 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -6 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -5 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -4 0 0 0 0 i\n"
                                    "MouseMoveEvent 110 -3 0 0 0 0 i\n"
                                    "MouseMoveEvent 109 -1 0 0 0 0 i\n"
                                    "MouseMoveEvent 109 0 0 0 0 0 i\n"
                                    "MouseMoveEvent 109 2 0 0 0 0 i\n"
                                    "MouseMoveEvent 109 3 0 0 0 0 i\n"
                                    "MouseMoveEvent 108 4 0 0 0 0 i\n"
                                    "MouseMoveEvent 108 6 0 0 0 0 i\n"
                                    "MouseMoveEvent 108 8 0 0 0 0 i\n"
                                    "MouseMoveEvent 108 10 0 0 0 0 i\n"
                                    "MouseMoveEvent 107 12 0 0 0 0 i\n"
                                    "MouseMoveEvent 107 14 0 0 0 0 i\n"
                                    "MouseMoveEvent 107 16 0 0 0 0 i\n"
                                    "MouseMoveEvent 107 18 0 0 0 0 i\n"
                                    "MouseMoveEvent 107 19 0 0 0 0 i\n"
                                    "MouseMoveEvent 107 21 0 0 0 0 i\n"
                                    "MouseMoveEvent 106 23 0 0 0 0 i\n"
                                    "MouseMoveEvent 106 24 0 0 0 0 i\n"
                                    "MouseMoveEvent 106 26 0 0 0 0 i\n"
                                    "MouseMoveEvent 106 28 0 0 0 0 i\n"
                                    "MouseMoveEvent 106 30 0 0 0 0 i\n"
                                    "MouseMoveEvent 106 32 0 0 0 0 i\n"
                                    "MouseMoveEvent 106 34 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 34 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 36 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 37 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 41 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 42 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 43 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 45 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 46 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 47 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 48 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 49 0 0 0 0 i\n"
                                    "MouseMoveEvent 105 50 0 0 0 0 i\n"
                                    "MouseMoveEvent 104 50 0 0 0 0 i\n"
                                    "MouseMoveEvent 104 51 0 0 0 0 i\n"
                                    "MouseMoveEvent 104 52 0 0 0 0 i\n"
                                    "MouseMoveEvent 104 53 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 53 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 54 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 55 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 56 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 57 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 58 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 59 0 0 0 0 i\n"
                                    "MouseMoveEvent 103 60 0 0 0 0 i\n"
                                    "MouseMoveEvent 102 60 0 0 0 0 i\n"
                                    "MouseMoveEvent 102 61 0 0 0 0 i\n"
                                    "MouseMoveEvent 102 62 0 0 0 0 i\n"
                                    "MouseMoveEvent 102 63 0 0 0 0 i\n"
                                    "MouseMoveEvent 102 64 0 0 0 0 i\n"
                                    "RightButtonReleaseEvent 102 64 0 0 0 0 i\n"
                                    "MouseMoveEvent 102 64 0 0 0 0 i\n"
                                    "MouseMoveEvent 102 64 0 0 0 0 i\n"
                                    "KeyPressEvent 102 64 0 0 101 1 e\n"
                                    "CharEvent 102 64 0 0 101 1 e\n"
                                    "ExitEvent 102 64 0 0 101 1 e\n";

int TestSphereWidgetZoomInOut(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0); // Turn off anti-aliasing
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkSphereWidget> sphWidget = svtkSmartPointer<svtkSphereWidget>::New();
  sphWidget->SetInteractor(iren);
  sphWidget->SetPlaceFactor(1.25);

  renderer->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // Callback
  svtkSmartPointer<svtkSphWCallback> myCallback = svtkSmartPointer<svtkSphWCallback>::New();
  sphWidget->AddObserver(svtkCommand::InteractionEvent, myCallback);

  // record events
  svtkSmartPointer<svtkInteractorEventRecorder> recorder =
    svtkSmartPointer<svtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  // recorder->SetFileName("c:/d/record.log");
  // recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(SphereWidgetEventLog);

  // interact with data
  // render the image
  //
  iren->Initialize();

  renWin->Render();
  recorder->Play();
  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();
  renWin->Render();

  // int retVal = svtkRegressionTestImageThreshold(renWin, 70);
  int retVal = svtkRegressionTestImage(renWin);

  iren->Start();

  return !retVal;
}
