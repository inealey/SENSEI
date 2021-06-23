/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAngleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAngleRepresentation3D.h"
#include "svtkActor.h"
#include "svtkArcSource.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkFollower.h"
#include "svtkInteractorObserver.h"
#include "svtkLineSource.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkVectorText.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkAngleRepresentation3D);

//----------------------------------------------------------------------
svtkAngleRepresentation3D::svtkAngleRepresentation3D()
{
  this->Angle = 0.0;

  // By default, use one of these handles
  this->HandleRepresentation = svtkPointHandleRepresentation3D::New();

  // Represent the line1
  this->Line1Source = svtkLineSource::New();
  this->Line1Source->SetResolution(5);
  this->Line1Mapper = svtkPolyDataMapper::New();
  this->Line1Mapper->SetInputConnection(this->Line1Source->GetOutputPort());
  this->Ray1 = svtkActor::New();
  this->Ray1->SetMapper(this->Line1Mapper);
  this->Ray1->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Represent the line2
  this->Line2Source = svtkLineSource::New();
  this->Line2Source->SetResolution(5);
  this->Line2Mapper = svtkPolyDataMapper::New();
  this->Line2Mapper->SetInputConnection(this->Line2Source->GetOutputPort());
  this->Ray2 = svtkActor::New();
  this->Ray2->SetMapper(this->Line2Mapper);
  this->Ray2->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Represent the arc
  this->ArcSource = svtkArcSource::New();
  this->ArcSource->SetResolution(30);
  this->ArcMapper = svtkPolyDataMapper::New();
  this->ArcMapper->SetInputConnection(this->ArcSource->GetOutputPort());
  this->Arc = svtkActor::New();
  this->Arc->SetMapper(this->ArcMapper);
  this->Arc->GetProperty()->SetColor(1.0, 0.1, 0.0);

  this->TextInput = svtkVectorText::New();
  this->TextInput->SetText("0");
  this->TextMapper = svtkPolyDataMapper::New();
  this->TextMapper->SetInputConnection(this->TextInput->GetOutputPort());
  this->TextActor = svtkFollower::New();
  this->TextActor->SetMapper(this->TextMapper);
  this->TextActor->GetProperty()->SetColor(1.0, 0.1, 0.0);
  this->ScaleInitialized = false;
}

//----------------------------------------------------------------------
svtkAngleRepresentation3D::~svtkAngleRepresentation3D()
{
  this->Line2Source->Delete();
  this->Line1Source->Delete();
  this->ArcSource->Delete();
  this->Line1Mapper->Delete();
  this->Line2Mapper->Delete();
  this->ArcMapper->Delete();
  this->Ray1->Delete();
  this->Ray2->Delete();
  this->Arc->Delete();
  this->TextInput->Delete();
  this->TextMapper->Delete();
  this->TextActor->Delete();
}

//----------------------------------------------------------------------
double svtkAngleRepresentation3D::GetAngle()
{
  return this->Angle;
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::GetPoint1WorldPosition(double pos[3])
{
  if (this->Point1Representation)
  {
    this->Point1Representation->GetWorldPosition(pos);
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::GetCenterWorldPosition(double pos[3])
{
  if (this->CenterRepresentation)
  {
    this->CenterRepresentation->GetWorldPosition(pos);
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::GetPoint2WorldPosition(double pos[3])
{
  if (this->Point2Representation)
  {
    this->Point2Representation->GetWorldPosition(pos);
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::SetPoint1WorldPosition(double x[3])
{
  if (!this->Point1Representation)
  {
    svtkErrorMacro("SetPoint1WorldPosition: null point 1 representation");
    return;
  }
  this->Point1Representation->SetWorldPosition(x);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::SetCenterWorldPosition(double x[3])
{
  if (!this->CenterRepresentation)
  {
    svtkErrorMacro("SetCenterWorldPosition: null center representation");
    return;
  }
  this->CenterRepresentation->SetWorldPosition(x);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::SetPoint2WorldPosition(double x[3])
{
  if (!this->Point2Representation)
  {
    svtkErrorMacro("SetPoint2WorldPosition: null point 2 representation");
    return;
  }
  this->Point2Representation->SetWorldPosition(x);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::SetPoint1DisplayPosition(double x[3])
{
  if (!this->Point1Representation)
  {
    svtkErrorMacro("SetPoint1DisplayPosition: null point 1 representation");
    return;
  }
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::SetCenterDisplayPosition(double x[3])
{
  if (!this->CenterRepresentation)
  {
    svtkErrorMacro("SetCenterDisplayPosition: null center point representation");
    return;
  }
  this->CenterRepresentation->SetDisplayPosition(x);
  double p[3];
  this->CenterRepresentation->GetWorldPosition(p);
  this->CenterRepresentation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::SetPoint2DisplayPosition(double x[3])
{
  if (!this->Point2Representation)
  {
    svtkErrorMacro("SetPoint2DisplayPosition: null point 2 representation");
    return;
  }
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::GetPoint1DisplayPosition(double pos[3])
{
  if (this->Point1Representation)
  {
    this->Point1Representation->GetDisplayPosition(pos);
    pos[2] = 0.0;
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::GetCenterDisplayPosition(double pos[3])
{
  if (this->CenterRepresentation)
  {
    this->CenterRepresentation->GetDisplayPosition(pos);
    pos[2] = 0.0;
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::GetPoint2DisplayPosition(double pos[3])
{
  if (this->Point2Representation)
  {
    this->Point2Representation->GetDisplayPosition(pos);
    pos[2] = 0.0;
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::BuildRepresentation()
{
  if (this->Point1Representation == nullptr || this->CenterRepresentation == nullptr ||
    this->Point2Representation == nullptr || this->ArcSource == nullptr)
  {
    // for now, return. Could create defaults here.
    return;
  }
  if (this->GetMTime() <= this->BuildTime ||
    this->Point1Representation->GetMTime() > this->BuildTime ||
    this->CenterRepresentation->GetMTime() > this->BuildTime ||
    this->Point2Representation->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    this->Superclass::BuildRepresentation();

    double p1[3], p2[3], c[3], p1d[3], p2d[3], cd[3], vector2[3], vector1[3];
    this->Point1Representation->GetWorldPosition(p1);
    this->CenterRepresentation->GetWorldPosition(c);
    this->Point2Representation->GetWorldPosition(p2);
    this->Point1Representation->GetDisplayPosition(p1d);
    this->CenterRepresentation->GetDisplayPosition(cd);
    this->Point2Representation->GetDisplayPosition(p2d);

    // Update the lines
    this->Line1Source->SetPoint1(p1);
    this->Line1Source->SetPoint2(c);
    this->Line2Source->SetPoint1(c);
    this->Line2Source->SetPoint2(p2);

    double l1 = 0.0, l2 = 0.0;
    // Compute the angle (only if necessary since we don't want
    // fluctuations in angle value as the camera moves, etc.)
    if (p1[0] - c[0] == 0.0 || p2[0] - c[0] == 0.0)
    {
      return;
    }

    vector1[0] = p1[0] - c[0];
    vector1[1] = p1[1] - c[1];
    vector1[2] = p1[2] - c[2];
    vector2[0] = p2[0] - c[0];
    vector2[1] = p2[1] - c[1];
    vector2[2] = p2[2] - c[2];
    l1 = svtkMath::Normalize(vector1);
    l2 = svtkMath::Normalize(vector2);
    this->Angle = acos(svtkMath::Dot(vector1, vector2));

    // Place the label and place the arc

    // If too small or no render get out
    if (!this->Renderer)
    {
      this->ArcVisibility = 0;
      return;
    }

    const double length = l1 < l2 ? l1 : l2;
    const double anglePlacementRatio = 0.5;
    const double l = length * anglePlacementRatio;
    double arcp1[3] = { l * vector1[0] + c[0], l * vector1[1] + c[1], l * vector1[2] + c[2] };
    double arcp2[3] = { l * vector2[0] + c[0], l * vector2[1] + c[1], l * vector2[2] + c[2] };
    this->ArcSource->SetPoint1(arcp1);
    this->ArcSource->SetPoint2(arcp2);
    this->ArcSource->SetCenter(c);
    if (this->Ray1Visibility && this->Ray2Visibility)
    {
      this->ArcSource->Update();

      svtkPoints* points = this->ArcSource->GetOutput()->GetPoints();
      const int npoints = points->GetNumberOfPoints();
      points->GetPoint(npoints / 2, this->TextPosition);

      char string[512];
      snprintf(string, sizeof(string), this->LabelFormat, svtkMath::DegreesFromRadians(this->Angle));

      this->TextInput->SetText(string);
      this->TextActor->SetCamera(this->Renderer->GetActiveCamera());
      this->TextActor->SetPosition(this->TextPosition);

      if (!this->ScaleInitialized)
      {
        // If a font size hasn't been specified by the user, scale the text
        // (font size) according to the length of the shortest arm of the
        // angle measurement.
        this->TextActor->SetScale(length / 10.0, length / 10.0, length / 10.0);
      }
    }

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::SetTextActorScale(double scale[3])
{
  this->TextActor->SetScale(scale);
  this->ScaleInitialized = true;
}

//----------------------------------------------------------------------
double* svtkAngleRepresentation3D::GetTextActorScale()
{
  return this->TextActor->GetScale();
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::ReleaseGraphicsResources(svtkWindow* w)
{
  this->Ray1->ReleaseGraphicsResources(w);
  this->Ray2->ReleaseGraphicsResources(w);
  this->Arc->ReleaseGraphicsResources(w);
  this->TextActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int svtkAngleRepresentation3D::RenderOpaqueGeometry(svtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  if (this->Ray1Visibility)
  {
    count += this->Ray1->RenderOpaqueGeometry(v);
  }
  if (this->Ray2Visibility)
  {
    count += this->Ray2->RenderOpaqueGeometry(v);
  }
  if (this->ArcVisibility)
  {
    count += this->Arc->RenderOpaqueGeometry(v);
  }
  if (this->Ray1Visibility && this->Ray2Visibility)
  {
    count += this->TextActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//----------------------------------------------------------------------
int svtkAngleRepresentation3D::RenderTranslucentPolygonalGeometry(svtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  if (this->Ray1Visibility)
  {
    count += this->Ray1->RenderTranslucentPolygonalGeometry(v);
  }
  if (this->Ray2Visibility)
  {
    count += this->Ray2->RenderTranslucentPolygonalGeometry(v);
  }
  if (this->ArcVisibility)
  {
    count += this->Arc->RenderTranslucentPolygonalGeometry(v);
  }
  if (this->Ray1Visibility && this->Ray2Visibility)
  {
    count += this->TextActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkAngleRepresentation3D::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  this->BuildRepresentation();
  result |= this->Ray1->HasTranslucentPolygonalGeometry();
  result |= this->Ray2->HasTranslucentPolygonalGeometry();
  result |= this->Arc->HasTranslucentPolygonalGeometry();
  result |= this->TextActor->HasTranslucentPolygonalGeometry();
  return result;
}

//----------------------------------------------------------------------
void svtkAngleRepresentation3D::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Ray1: ";
  if (this->Ray1)
  {
    this->Ray1->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Ray2: ";
  if (this->Ray2)
  {
    this->Ray2->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Arc: ";
  if (this->Arc)
  {
    this->Arc->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "TextActor: ";
  if (this->TextActor)
  {
    this->TextActor->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
