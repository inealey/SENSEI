/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLightRepresentation.h"

#include "svtkActor.h"
#include "svtkBox.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkConeSource.h"
#include "svtkInteractorObserver.h"
#include "svtkLineSource.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkLightRepresentation);

//----------------------------------------------------------------------
svtkLightRepresentation::svtkLightRepresentation()
{
  // Initialize state
  this->InteractionState = svtkLightRepresentation::Outside;
  this->HandleSize = 10.0;
  this->InitialLength = 1;
  this->ValidPick = 1;

  // Set up the initial properties
  this->Property->SetAmbient(1.0);
  this->Property->SetColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(0.5);
  this->Property->SetRepresentationToWireframe();

  // Represent the sphere
  this->Sphere->LatLongTessellationOn();
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper->SetInputConnection(this->Sphere->GetOutputPort());
  this->SphereActor->SetMapper(this->SphereMapper);
  this->SphereActor->SetProperty(this->Property);

  // Sphere picking
  this->SpherePicker->PickFromListOn();
  this->SpherePicker->AddPickList(this->SphereActor);
  this->SpherePicker->SetTolerance(0.01); // need some fluff

  // Represent the Cone
  this->ConeMapper->SetInputConnection(this->Cone->GetOutputPort());
  this->ConeActor->SetMapper(this->ConeMapper);
  this->ConeActor->SetProperty(this->Property);

  // Cone picking
  this->ConePicker->PickFromListOn();
  this->ConePicker->AddPickList(this->ConeActor);
  this->ConePicker->SetTolerance(0.01); // need some fluff

  // Represent the Line
  this->LineMapper->SetInputConnection(this->Line->GetOutputPort());
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->SetProperty(this->Property);

  // Line picking
  this->LinePicker->PickFromListOn();
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->SetTolerance(0.01); // need some fluff

  // Update the representation sources
  this->UpdateSources();
}

//----------------------------------------------------------------------
svtkLightRepresentation::~svtkLightRepresentation()
{
  // Needed in order to be able to forward declare the classes in svtkNew
}

//----------------------------------------------------------------------
void svtkLightRepresentation::SetLightPosition(double x[3])
{
  if (this->LightPosition[0] != x[0] || this->LightPosition[1] != x[1] ||
    this->LightPosition[2] != x[2])
  {
    this->LightPosition[0] = x[0];
    this->LightPosition[1] = x[1];
    this->LightPosition[2] = x[2];
    this->UpdateSources();
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkLightRepresentation::SetFocalPoint(double x[3])
{
  if (this->FocalPoint[0] != x[0] || this->FocalPoint[1] != x[1] || this->FocalPoint[2] != x[2])
  {
    this->FocalPoint[0] = x[0];
    this->FocalPoint[1] = x[1];
    this->FocalPoint[2] = x[2];
    this->UpdateSources();
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkLightRepresentation::SetConeAngle(double angle)
{
  // Clamp between 0 and 89.98 because of
  // https://gitlab.kitware.com/paraview/paraview/issues/19223
  angle = svtkMath::ClampValue(angle, 0.0, 89.98);
  if (this->ConeAngle != angle)
  {
    this->ConeAngle = angle;
    this->UpdateSources();
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkLightRepresentation::SetLightColor(double* color)
{
  this->Property->SetColor(color);
}

//----------------------------------------------------------------------
double* svtkLightRepresentation::GetLightColor()
{
  return this->Property->GetColor();
}

//----------------------------------------------------------------------
void svtkLightRepresentation::UpdateSources()
{
  this->Sphere->SetCenter(this->LightPosition);
  this->Line->SetPoint1(this->LightPosition);
  this->Line->SetPoint2(this->FocalPoint);

  double vec[3];
  svtkMath::Subtract(this->LightPosition, this->FocalPoint, vec);
  double center[3];
  svtkMath::Add(this->LightPosition, this->FocalPoint, center);
  svtkMath::MultiplyScalar(center, 0.5);
  double height = svtkMath::Norm(vec);

  this->Cone->SetCenter(center);
  this->Cone->SetHeight(height);
  this->Cone->SetDirection(vec);
  this->Cone->SetRadius(std::tan(svtkMath::Pi() * this->ConeAngle / 180) * height);

  this->Sphere->Update();
  this->Line->Update();
  this->Cone->Update();
  this->SizeHandles();
}

//----------------------------------------------------------------------
double* svtkLightRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->SphereActor->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());
  if (this->Positional)
  {
    this->BoundingBox->AddBounds(this->ConeActor->GetBounds());
  }
  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------
void svtkLightRepresentation::StartWidgetInteraction(double eventPosition[2])
{
  // Store the start position
  this->StartEventPosition[0] = eventPosition[0];
  this->StartEventPosition[1] = eventPosition[1];
  this->StartEventPosition[2] = 0.0;

  // Store the last position
  this->LastEventPosition[0] = eventPosition[0];
  this->LastEventPosition[1] = eventPosition[1];
  this->LastEventPosition[2] = 0.0;

  // Initialize scaling distance
  this->LastScalingDistance2 = -1;
}

//----------------------------------------------------------------------
void svtkLightRepresentation::WidgetInteraction(double eventPosition[2])
{
  // Convert events to appropriate coordinate systems
  svtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  double lookPoint[4], pickPoint[4];
  double vpn[3];
  camera->GetViewPlaneNormal(vpn);

  // Compute the two points defining the motion vector
  double pos[3];
  this->LastPicker->GetPickPosition(pos);
  svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2], lookPoint);
  double z = lookPoint[2];
  svtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, eventPosition[0], eventPosition[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == svtkLightRepresentation::MovingLight)
  {
    this->SetLightPosition(pickPoint);
  }
  else if (this->InteractionState == svtkLightRepresentation::MovingFocalPoint ||
    this->InteractionState == svtkLightRepresentation::MovingPositionalFocalPoint)
  {
    this->SetFocalPoint(pickPoint);
  }
  else if (this->InteractionState == svtkLightRepresentation::ScalingConeAngle)
  {
    // Recover last picked point
    double lastPickPoint[4];
    svtkInteractorObserver::ComputeDisplayToWorld(
      this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, lastPickPoint);

    // Scale the cone angle
    this->ScaleConeAngle(pickPoint, lastPickPoint);
  }

  // Store the start position
  this->LastEventPosition[0] = eventPosition[0];
  this->LastEventPosition[1] = eventPosition[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
int svtkLightRepresentation::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  // Picked point is not inside
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    this->InteractionState = svtkLightRepresentation::Outside;
    return this->InteractionState;
  }

  // Check if sphere is picked
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->SpherePicker);
  if (path != nullptr)
  {
    this->LastPicker = this->SpherePicker;
    this->InteractionState = svtkLightRepresentation::MovingLight;
    return this->InteractionState;
  }

  if (this->Positional)
  {
    // Check if cone is picked
    path = this->GetAssemblyPath(X, Y, 0., this->ConePicker);
    if (path != nullptr)
    {
      this->LastPicker = this->ConePicker;
      this->InteractionState = svtkLightRepresentation::MovingPositionalFocalPoint;
      return this->InteractionState;
    }
  }
  else
  {
    // Check if line is picked
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);
    if (path != nullptr)
    {
      this->LastPicker = this->LinePicker;
      this->InteractionState = svtkLightRepresentation::MovingFocalPoint;
      return this->InteractionState;
    }
  }

  this->InteractionState = svtkLightRepresentation::Outside;
  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkLightRepresentation::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer &&
      ((this->Renderer->GetSVTKWindow() &&
         this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime) ||
        (this->Renderer->GetActiveCamera() &&
          this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime))))
  {
    // resize the handles
    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkLightRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->SphereActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
  this->ConeActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int svtkLightRepresentation::RenderOpaqueGeometry(svtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  count += this->SphereActor->RenderOpaqueGeometry(v);
  count += this->LineActor->RenderOpaqueGeometry(v);
  if (this->Positional)
  {
    count += this->ConeActor->RenderOpaqueGeometry(v);
  }
  return count;
}

//----------------------------------------------------------------------
int svtkLightRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  count += this->SphereActor->RenderTranslucentPolygonalGeometry(v);
  count += this->LineActor->RenderTranslucentPolygonalGeometry(v);
  if (this->Positional)
  {
    count += this->ConeActor->RenderTranslucentPolygonalGeometry(v);
  }
  return count;
}

//----------------------------------------------------------------------
void svtkLightRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "LightPosition: " << this->LightPosition[0] << " " << this->LightPosition[1]
     << " " << this->LightPosition[2] << endl;
  os << indent << "FocalPoint: " << this->FocalPoint[0] << " " << this->FocalPoint[1] << " "
     << this->FocalPoint[2] << endl;
  os << indent << "ConeAngle: " << this->ConeAngle << endl;
  os << indent << "Positional: " << this->Positional << endl;

  os << indent << "Property: ";
  this->Property->PrintSelf(os, indent.GetNextIndent());

  os << indent << "BoundingBox: ";
  this->BoundingBox->PrintSelf(os, indent.GetNextIndent());

  os << indent << "LastScalingDistance2: " << this->LastScalingDistance2 << endl;
  os << indent << "LastEventPosition: " << this->LastEventPosition[0] << " "
     << this->LastEventPosition[1] << " " << this->LastEventPosition[2] << endl;

  os << indent << "Sphere: ";
  this->Sphere->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SphereActor: ";
  this->SphereActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SphereMapper: ";
  this->SphereMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SpherePicker: ";
  this->SpherePicker->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Line: ";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LineActor: ";
  this->LineActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LineMapper: ";
  this->LineMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LinePicker: ";
  this->LinePicker->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Cone: ";
  this->Cone->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ConeActor: ";
  this->ConeActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ConeMapper: ";
  this->ConeMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ConePicker: ";
  this->ConePicker->PrintSelf(os, indent.GetNextIndent());

  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkLightRepresentation::SizeHandles()
{
  double radius =
    this->svtkWidgetRepresentation::SizeHandlesInPixels(1.5, this->Sphere->GetOutput()->GetCenter());
  this->Sphere->SetRadius(radius);
}

//----------------------------------------------------------------------------
void svtkLightRepresentation::ScaleConeAngle(double* pickPoint, double* lastPickPoint)
{
  double vecOrig[3];
  double vecCur[3];
  double vecPrev[3];
  double project[3];

  // Compute the squated distance from the picked point
  svtkMath::Subtract(this->FocalPoint, this->LightPosition, vecOrig);
  svtkMath::Subtract(pickPoint, this->LightPosition, vecCur);
  svtkMath::Subtract(lastPickPoint, this->LightPosition, vecPrev);
  svtkMath::ProjectVector(vecCur, vecOrig, project);
  double distance2 = svtkMath::Distance2BetweenPoints(pickPoint, project);

  // If a squared distance has been computed before, the angle has changed
  if (this->LastScalingDistance2 != -1)
  {
    // Compute the direction of the angle change
    double factor = this->LastScalingDistance2 < distance2 ? 1 : -1;

    // Compute the difference of the change
    double deltaAngle =
      factor * 180 * svtkMath::AngleBetweenVectors(vecCur, vecPrev) / svtkMath::Pi();

    // Add it to the current angle
    this->SetConeAngle(this->ConeAngle + deltaAngle);
  }

  // Store the last scaling squared distance
  this->LastScalingDistance2 = distance2;
}
