//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/Camera.h>

namespace svtkm
{
namespace rendering
{

svtkm::Matrix<svtkm::Float32, 4, 4> Camera::Camera3DStruct::CreateViewMatrix() const
{
  return MatrixHelpers::ViewMatrix(this->Position, this->LookAt, this->ViewUp);
}

svtkm::Matrix<svtkm::Float32, 4, 4> Camera::Camera3DStruct::CreateProjectionMatrix(
  svtkm::Id width,
  svtkm::Id height,
  svtkm::Float32 nearPlane,
  svtkm::Float32 farPlane) const
{
  svtkm::Matrix<svtkm::Float32, 4, 4> matrix;
  svtkm::MatrixIdentity(matrix);

  svtkm::Float32 AspectRatio = svtkm::Float32(width) / svtkm::Float32(height);
  svtkm::Float32 fovRad = this->FieldOfView * svtkm::Pi_180f();
  fovRad = svtkm::Tan(fovRad * 0.5f);
  svtkm::Float32 size = nearPlane * fovRad;
  svtkm::Float32 left = -size * AspectRatio;
  svtkm::Float32 right = size * AspectRatio;
  svtkm::Float32 bottom = -size;
  svtkm::Float32 top = size;

  matrix(0, 0) = 2.f * nearPlane / (right - left);
  matrix(1, 1) = 2.f * nearPlane / (top - bottom);
  matrix(0, 2) = (right + left) / (right - left);
  matrix(1, 2) = (top + bottom) / (top - bottom);
  matrix(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
  matrix(3, 2) = -1.f;
  matrix(2, 3) = -(2.f * farPlane * nearPlane) / (farPlane - nearPlane);
  matrix(3, 3) = 0.f;

  svtkm::Matrix<svtkm::Float32, 4, 4> T, Z;
  T = svtkm::Transform3DTranslate(this->XPan, this->YPan, 0.f);
  Z = svtkm::Transform3DScale(this->Zoom, this->Zoom, 1.f);
  matrix = svtkm::MatrixMultiply(Z, svtkm::MatrixMultiply(T, matrix));
  return matrix;
}

//---------------------------------------------------------------------------

svtkm::Matrix<svtkm::Float32, 4, 4> Camera::Camera2DStruct::CreateViewMatrix() const
{
  svtkm::Vec3f_32 lookAt((this->Left + this->Right) / 2.f, (this->Top + this->Bottom) / 2.f, 0.f);
  svtkm::Vec3f_32 position = lookAt;
  position[2] = 1.f;
  svtkm::Vec3f_32 up(0, 1, 0);
  svtkm::Matrix<svtkm::Float32, 4, 4> V = MatrixHelpers::ViewMatrix(position, lookAt, up);
  svtkm::Matrix<svtkm::Float32, 4, 4> scaleMatrix = MatrixHelpers::CreateScale(this->XScale, 1, 1);
  V = svtkm::MatrixMultiply(scaleMatrix, V);
  return V;
}

svtkm::Matrix<svtkm::Float32, 4, 4> Camera::Camera2DStruct::CreateProjectionMatrix(
  svtkm::Float32 size,
  svtkm::Float32 znear,
  svtkm::Float32 zfar,
  svtkm::Float32 aspect) const
{
  svtkm::Matrix<svtkm::Float32, 4, 4> matrix(0.f);
  svtkm::Float32 left = -size / 2.f * aspect;
  svtkm::Float32 right = size / 2.f * aspect;
  svtkm::Float32 bottom = -size / 2.f;
  svtkm::Float32 top = size / 2.f;

  matrix(0, 0) = 2.f / (right - left);
  matrix(1, 1) = 2.f / (top - bottom);
  matrix(2, 2) = -2.f / (zfar - znear);
  matrix(0, 3) = -(right + left) / (right - left);
  matrix(1, 3) = -(top + bottom) / (top - bottom);
  matrix(2, 3) = -(zfar + znear) / (zfar - znear);
  matrix(3, 3) = 1.f;

  svtkm::Matrix<svtkm::Float32, 4, 4> T, Z;
  T = svtkm::Transform3DTranslate(this->XPan, this->YPan, 0.f);
  Z = svtkm::Transform3DScale(this->Zoom, this->Zoom, 1.f);
  matrix = svtkm::MatrixMultiply(Z, svtkm::MatrixMultiply(T, matrix));
  return matrix;
}

//---------------------------------------------------------------------------

svtkm::Matrix<svtkm::Float32, 4, 4> Camera::CreateViewMatrix() const
{
  if (this->Mode == Camera::MODE_3D)
  {
    return this->Camera3D.CreateViewMatrix();
  }
  else
  {
    return this->Camera2D.CreateViewMatrix();
  }
}

svtkm::Matrix<svtkm::Float32, 4, 4> Camera::CreateProjectionMatrix(svtkm::Id screenWidth,
                                                                 svtkm::Id screenHeight) const
{
  if (this->Mode == Camera::MODE_3D)
  {
    return this->Camera3D.CreateProjectionMatrix(
      screenWidth, screenHeight, this->NearPlane, this->FarPlane);
  }
  else
  {
    svtkm::Float32 size = svtkm::Abs(this->Camera2D.Top - this->Camera2D.Bottom);
    svtkm::Float32 left, right, bottom, top;
    this->GetRealViewport(screenWidth, screenHeight, left, right, bottom, top);
    svtkm::Float32 aspect = (static_cast<svtkm::Float32>(screenWidth) * (right - left)) /
      (static_cast<svtkm::Float32>(screenHeight) * (top - bottom));

    return this->Camera2D.CreateProjectionMatrix(size, this->NearPlane, this->FarPlane, aspect);
  }
}

void Camera::GetRealViewport(svtkm::Id screenWidth,
                             svtkm::Id screenHeight,
                             svtkm::Float32& left,
                             svtkm::Float32& right,
                             svtkm::Float32& bottom,
                             svtkm::Float32& top) const
{
  if (this->Mode == Camera::MODE_3D)
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  else
  {
    svtkm::Float32 maxvw =
      (this->ViewportRight - this->ViewportLeft) * static_cast<svtkm::Float32>(screenWidth);
    svtkm::Float32 maxvh =
      (this->ViewportTop - this->ViewportBottom) * static_cast<svtkm::Float32>(screenHeight);
    svtkm::Float32 waspect = maxvw / maxvh;
    svtkm::Float32 daspect =
      (this->Camera2D.Right - this->Camera2D.Left) / (this->Camera2D.Top - this->Camera2D.Bottom);
    daspect *= this->Camera2D.XScale;
//cerr << "waspect="<<waspect << "   \tdaspect="<<daspect<<endl;

//needed as center is a constant value
#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif

    const bool center = true; // if false, anchor to bottom-left
    if (waspect > daspect)
    {
      svtkm::Float32 new_w = (this->ViewportRight - this->ViewportLeft) * daspect / waspect;
      if (center)
      {
        left = (this->ViewportLeft + this->ViewportRight) / 2.f - new_w / 2.f;
        right = (this->ViewportLeft + this->ViewportRight) / 2.f + new_w / 2.f;
      }
      else
      {
        left = this->ViewportLeft;
        right = this->ViewportLeft + new_w;
      }
      bottom = this->ViewportBottom;
      top = this->ViewportTop;
    }
    else
    {
      svtkm::Float32 new_h = (this->ViewportTop - this->ViewportBottom) * waspect / daspect;
      if (center)
      {
        bottom = (this->ViewportBottom + this->ViewportTop) / 2.f - new_h / 2.f;
        top = (this->ViewportBottom + this->ViewportTop) / 2.f + new_h / 2.f;
      }
      else
      {
        bottom = this->ViewportBottom;
        top = this->ViewportBottom + new_h;
      }
      left = this->ViewportLeft;
      right = this->ViewportRight;
    }
#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
  }
}

void Camera::Pan(svtkm::Float32 dx, svtkm::Float32 dy)
{
  this->Camera3D.XPan += dx;
  this->Camera3D.YPan += dy;
  this->Camera2D.XPan += dx;
  this->Camera2D.YPan += dy;
}

void Camera::Zoom(svtkm::Float32 zoom)
{
  svtkm::Float32 factor = svtkm::Pow(4.0f, zoom);
  this->Camera3D.Zoom *= factor;
  this->Camera3D.XPan *= factor;
  this->Camera3D.YPan *= factor;
  this->Camera2D.Zoom *= factor;
  this->Camera2D.XPan *= factor;
  this->Camera2D.YPan *= factor;
}

void Camera::TrackballRotate(svtkm::Float32 startX,
                             svtkm::Float32 startY,
                             svtkm::Float32 endX,
                             svtkm::Float32 endY)
{
  svtkm::Matrix<svtkm::Float32, 4, 4> rotate =
    MatrixHelpers::TrackballMatrix(startX, startY, endX, endY);

  //Translate matrix
  svtkm::Matrix<svtkm::Float32, 4, 4> translate = svtkm::Transform3DTranslate(-this->Camera3D.LookAt);

  //Translate matrix
  svtkm::Matrix<svtkm::Float32, 4, 4> inverseTranslate =
    svtkm::Transform3DTranslate(this->Camera3D.LookAt);

  svtkm::Matrix<svtkm::Float32, 4, 4> view = this->CreateViewMatrix();
  view(0, 3) = 0;
  view(1, 3) = 0;
  view(2, 3) = 0;

  svtkm::Matrix<svtkm::Float32, 4, 4> inverseView = svtkm::MatrixTranspose(view);

  //fullTransform = inverseTranslate * inverseView * rotate * view * translate
  svtkm::Matrix<svtkm::Float32, 4, 4> fullTransform;
  fullTransform = svtkm::MatrixMultiply(
    inverseTranslate,
    svtkm::MatrixMultiply(inverseView,
                         svtkm::MatrixMultiply(rotate, svtkm::MatrixMultiply(view, translate))));
  this->Camera3D.Position = svtkm::Transform3DPoint(fullTransform, this->Camera3D.Position);
  this->Camera3D.LookAt = svtkm::Transform3DPoint(fullTransform, this->Camera3D.LookAt);
  this->Camera3D.ViewUp = svtkm::Transform3DVector(fullTransform, this->Camera3D.ViewUp);
}

void Camera::ResetToBounds(const svtkm::Bounds& dataBounds,
                           const svtkm::Float64 XDataViewPadding,
                           const svtkm::Float64 YDataViewPadding,
                           const svtkm::Float64 ZDataViewPadding)
{
  // Save camera mode
  ModeEnum saveMode = this->GetMode();

  // Pad view around data extents
  svtkm::Bounds db = dataBounds;
  svtkm::Float64 viewPadAmount = XDataViewPadding * (db.X.Max - db.X.Min);
  db.X.Max += viewPadAmount;
  db.X.Min -= viewPadAmount;
  viewPadAmount = YDataViewPadding * (db.Y.Max - db.Y.Min);
  db.Y.Max += viewPadAmount;
  db.Y.Min -= viewPadAmount;
  viewPadAmount = ZDataViewPadding * (db.Z.Max - db.Z.Min);
  db.Z.Max += viewPadAmount;
  db.Z.Min -= viewPadAmount;

  // Reset for 3D camera
  svtkm::Vec3f_32 directionOfProjection = this->GetPosition() - this->GetLookAt();
  svtkm::Normalize(directionOfProjection);

  svtkm::Vec3f_32 center = db.Center();
  this->SetLookAt(center);

  svtkm::Vec3f_32 totalExtent;
  totalExtent[0] = svtkm::Float32(db.X.Length());
  totalExtent[1] = svtkm::Float32(db.Y.Length());
  totalExtent[2] = svtkm::Float32(db.Z.Length());
  svtkm::Float32 diagonalLength = svtkm::Magnitude(totalExtent);
  this->SetPosition(center + directionOfProjection * diagonalLength * 1.0f);
  this->SetFieldOfView(60.0f);
  this->SetClippingRange(0.1f * diagonalLength, diagonalLength * 10.0f);

  // Reset for 2D camera
  this->SetViewRange2D(db);

  // Reset pan and zoom
  this->Camera3D.XPan = 0;
  this->Camera3D.YPan = 0;
  this->Camera3D.Zoom = 1;
  this->Camera2D.XPan = 0;
  this->Camera2D.YPan = 0;
  this->Camera2D.Zoom = 1;

  // Restore camera mode
  this->SetMode(saveMode);
}

// Enable the ability to pad the data extents in the final view
void Camera::ResetToBounds(const svtkm::Bounds& dataBounds, const svtkm::Float64 dataViewPadding)
{
  Camera::ResetToBounds(dataBounds, dataViewPadding, dataViewPadding, dataViewPadding);
}

void Camera::ResetToBounds(const svtkm::Bounds& dataBounds)
{
  Camera::ResetToBounds(dataBounds, 0);
}

void Camera::Roll(svtkm::Float32 angleDegrees)
{
  svtkm::Vec3f_32 directionOfProjection = this->GetLookAt() - this->GetPosition();
  svtkm::Matrix<svtkm::Float32, 4, 4> rotateTransform =
    svtkm::Transform3DRotate(angleDegrees, directionOfProjection);

  this->SetViewUp(svtkm::Transform3DVector(rotateTransform, this->GetViewUp()));
}

void Camera::Azimuth(svtkm::Float32 angleDegrees)
{
  // Translate to the focal point (LookAt), rotate about view up, and
  // translate back again.
  svtkm::Matrix<svtkm::Float32, 4, 4> transform = svtkm::Transform3DTranslate(this->GetLookAt());
  transform =
    svtkm::MatrixMultiply(transform, svtkm::Transform3DRotate(angleDegrees, this->GetViewUp()));
  transform = svtkm::MatrixMultiply(transform, svtkm::Transform3DTranslate(-this->GetLookAt()));

  this->SetPosition(svtkm::Transform3DPoint(transform, this->GetPosition()));
}

void Camera::Elevation(svtkm::Float32 angleDegrees)
{
  svtkm::Vec3f_32 axisOfRotation =
    svtkm::Cross(this->GetPosition() - this->GetLookAt(), this->GetViewUp());

  // Translate to the focal point (LookAt), rotate about the defined axis,
  // and translate back again.
  svtkm::Matrix<svtkm::Float32, 4, 4> transform = svtkm::Transform3DTranslate(this->GetLookAt());
  transform =
    svtkm::MatrixMultiply(transform, svtkm::Transform3DRotate(angleDegrees, axisOfRotation));
  transform = svtkm::MatrixMultiply(transform, svtkm::Transform3DTranslate(-this->GetLookAt()));

  this->SetPosition(svtkm::Transform3DPoint(transform, this->GetPosition()));
}

void Camera::Dolly(svtkm::Float32 value)
{
  if (value <= svtkm::Epsilon32())
  {
    return;
  }

  svtkm::Vec3f_32 lookAtToPos = this->GetPosition() - this->GetLookAt();

  this->SetPosition(this->GetLookAt() + (1.0f / value) * lookAtToPos);
}

void Camera::Print() const
{
  if (Mode == MODE_3D)
  {
    std::cout << "Camera: 3D" << std::endl;
    std::cout << "  LookAt: " << Camera3D.LookAt << std::endl;
    std::cout << "  Pos   : " << Camera3D.Position << std::endl;
    std::cout << "  Up    : " << Camera3D.ViewUp << std::endl;
    std::cout << "  FOV   : " << GetFieldOfView() << std::endl;
    std::cout << "  Clip  : " << GetClippingRange() << std::endl;
    std::cout << "  XyZ   : " << Camera3D.XPan << " " << Camera3D.YPan << " " << Camera3D.Zoom
              << std::endl;
    svtkm::Matrix<svtkm::Float32, 4, 4> pm, vm;
    pm = CreateProjectionMatrix(512, 512);
    vm = CreateViewMatrix();
    std::cout << " PM: " << std::endl;
    std::cout << pm[0][0] << " " << pm[0][1] << " " << pm[0][2] << " " << pm[0][3] << std::endl;
    std::cout << pm[1][0] << " " << pm[1][1] << " " << pm[1][2] << " " << pm[1][3] << std::endl;
    std::cout << pm[2][0] << " " << pm[2][1] << " " << pm[2][2] << " " << pm[2][3] << std::endl;
    std::cout << pm[3][0] << " " << pm[3][1] << " " << pm[3][2] << " " << pm[3][3] << std::endl;
    std::cout << " VM: " << std::endl;
    std::cout << vm[0][0] << " " << vm[0][1] << " " << vm[0][2] << " " << vm[0][3] << std::endl;
    std::cout << vm[1][0] << " " << vm[1][1] << " " << vm[1][2] << " " << vm[1][3] << std::endl;
    std::cout << vm[2][0] << " " << vm[2][1] << " " << vm[2][2] << " " << vm[2][3] << std::endl;
    std::cout << vm[3][0] << " " << vm[3][1] << " " << vm[3][2] << " " << vm[3][3] << std::endl;
  }
  else if (Mode == MODE_2D)
  {
    std::cout << "Camera: 2D" << std::endl;
    std::cout << "  LRBT: " << Camera2D.Left << " " << Camera2D.Right << " " << Camera2D.Bottom
              << " " << Camera2D.Top << std::endl;
    std::cout << "  XY  : " << Camera2D.XPan << " " << Camera2D.YPan << std::endl;
    std::cout << "  SZ  : " << Camera2D.XScale << " " << Camera2D.Zoom << std::endl;
    svtkm::Matrix<svtkm::Float32, 4, 4> pm, vm;
    pm = CreateProjectionMatrix(512, 512);
    vm = CreateViewMatrix();
    std::cout << " PM: " << std::endl;
    std::cout << pm[0][0] << " " << pm[0][1] << " " << pm[0][2] << " " << pm[0][3] << std::endl;
    std::cout << pm[1][0] << " " << pm[1][1] << " " << pm[1][2] << " " << pm[1][3] << std::endl;
    std::cout << pm[2][0] << " " << pm[2][1] << " " << pm[2][2] << " " << pm[2][3] << std::endl;
    std::cout << pm[3][0] << " " << pm[3][1] << " " << pm[3][2] << " " << pm[3][3] << std::endl;
    std::cout << " VM: " << std::endl;
    std::cout << vm[0][0] << " " << vm[0][1] << " " << vm[0][2] << " " << vm[0][3] << std::endl;
    std::cout << vm[1][0] << " " << vm[1][1] << " " << vm[1][2] << " " << vm[1][3] << std::endl;
    std::cout << vm[2][0] << " " << vm[2][1] << " " << vm[2][2] << " " << vm[2][3] << std::endl;
    std::cout << vm[3][0] << " " << vm[3][1] << " " << vm[3][2] << " " << vm[3][3] << std::endl;
  }
}
}
} // namespace svtkm::rendering
