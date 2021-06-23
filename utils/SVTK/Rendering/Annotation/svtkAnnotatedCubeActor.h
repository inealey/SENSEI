/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAnnotatedCubeActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAnnotatedCubeActor
 * @brief   a 3D cube with face labels
 *
 * svtkAnnotatedCubeActor is a hybrid 3D actor used to represent an anatomical
 * orientation marker in a scene.  The class consists of a 3D unit cube centered
 * on the origin with each face labelled in correspondence to a particular
 * coordinate direction.  For example, with Cartesian directions, the user
 * defined text labels could be: +X, -X, +Y, -Y, +Z, -Z, while for anatomical
 * directions: A, P, L, R, S, I.  Text is automatically centered on each cube
 * face and is not restriceted to single characters. In addition to or in
 * replace of a solid text label representation, the outline edges of the labels
 * can be displayed.  The individual properties of the cube, face labels
 * and text outlines can be manipulated as can their visibility.
 *
 * @warning
 * svtkAnnotatedCubeActor is primarily intended for use with
 * svtkOrientationMarkerWidget. The cube face text is generated by svtkVectorText
 * and therefore the font attributes are restricted.
 *
 * @sa
 * svtkAxesActor svtkOrientationMarkerWidget svtkVectorText
 */

#ifndef svtkAnnotatedCubeActor_h
#define svtkAnnotatedCubeActor_h

#include "svtkProp3D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkActor;
class svtkAppendPolyData;
class svtkAssembly;
class svtkCubeSource;
class svtkFeatureEdges;
class svtkPropCollection;
class svtkProperty;
class svtkRenderer;
class svtkTransform;
class svtkTransformFilter;
class svtkVectorText;

class SVTKRENDERINGANNOTATION_EXPORT svtkAnnotatedCubeActor : public svtkProp3D
{
public:
  static svtkAnnotatedCubeActor* New();
  svtkTypeMacro(svtkAnnotatedCubeActor, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(svtkPropCollection*) override;

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Shallow copy of an axes actor. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  void GetBounds(double bounds[6]);
  double* GetBounds() SVTK_SIZEHINT(6) override;
  //@}

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get the scale factor for the face text
   */
  void SetFaceTextScale(double);
  svtkGetMacro(FaceTextScale, double);
  //@}

  //@{
  /**
   * Get the individual face text properties.
   */
  svtkProperty* GetXPlusFaceProperty();
  svtkProperty* GetXMinusFaceProperty();
  svtkProperty* GetYPlusFaceProperty();
  svtkProperty* GetYMinusFaceProperty();
  svtkProperty* GetZPlusFaceProperty();
  svtkProperty* GetZMinusFaceProperty();
  //@}

  /**
   * Get the cube properties.
   */
  svtkProperty* GetCubeProperty();

  /**
   * Get the text edges properties.
   */
  svtkProperty* GetTextEdgesProperty();

  //@{
  /**
   * Set/get the face text.
   */
  svtkSetStringMacro(XPlusFaceText);
  svtkGetStringMacro(XPlusFaceText);
  svtkSetStringMacro(XMinusFaceText);
  svtkGetStringMacro(XMinusFaceText);
  svtkSetStringMacro(YPlusFaceText);
  svtkGetStringMacro(YPlusFaceText);
  svtkSetStringMacro(YMinusFaceText);
  svtkGetStringMacro(YMinusFaceText);
  svtkSetStringMacro(ZPlusFaceText);
  svtkGetStringMacro(ZPlusFaceText);
  svtkSetStringMacro(ZMinusFaceText);
  svtkGetStringMacro(ZMinusFaceText);
  //@}

  //@{
  /**
   * Enable/disable drawing the vector text edges.
   */
  void SetTextEdgesVisibility(int);
  int GetTextEdgesVisibility();
  //@}

  //@{
  /**
   * Enable/disable drawing the cube.
   */
  void SetCubeVisibility(int);
  int GetCubeVisibility();
  //@}

  //@{
  /**
   * Enable/disable drawing the vector text.
   */
  void SetFaceTextVisibility(int);
  int GetFaceTextVisibility();
  //@}

  //@{
  /**
   * Augment individual face text orientations.
   */
  svtkSetMacro(XFaceTextRotation, double);
  svtkGetMacro(XFaceTextRotation, double);
  svtkSetMacro(YFaceTextRotation, double);
  svtkGetMacro(YFaceTextRotation, double);
  svtkSetMacro(ZFaceTextRotation, double);
  svtkGetMacro(ZFaceTextRotation, double);
  //@}

  /**
   * Get the assembly so that user supplied transforms can be applied
   */
  svtkAssembly* GetAssembly() { return this->Assembly; }

protected:
  svtkAnnotatedCubeActor();
  ~svtkAnnotatedCubeActor() override;

  svtkCubeSource* CubeSource;
  svtkActor* CubeActor;

  svtkAppendPolyData* AppendTextEdges;
  svtkFeatureEdges* ExtractTextEdges;
  svtkActor* TextEdgesActor;

  void UpdateProps();

  char* XPlusFaceText;
  char* XMinusFaceText;
  char* YPlusFaceText;
  char* YMinusFaceText;
  char* ZPlusFaceText;
  char* ZMinusFaceText;

  double FaceTextScale;

  double XFaceTextRotation;
  double YFaceTextRotation;
  double ZFaceTextRotation;

  svtkVectorText* XPlusFaceVectorText;
  svtkVectorText* XMinusFaceVectorText;
  svtkVectorText* YPlusFaceVectorText;
  svtkVectorText* YMinusFaceVectorText;
  svtkVectorText* ZPlusFaceVectorText;
  svtkVectorText* ZMinusFaceVectorText;

  svtkActor* XPlusFaceActor;
  svtkActor* XMinusFaceActor;
  svtkActor* YPlusFaceActor;
  svtkActor* YMinusFaceActor;
  svtkActor* ZPlusFaceActor;
  svtkActor* ZMinusFaceActor;

  svtkTransformFilter* InternalTransformFilter;
  svtkTransform* InternalTransform;

  svtkAssembly* Assembly;

private:
  svtkAnnotatedCubeActor(const svtkAnnotatedCubeActor&) = delete;
  void operator=(const svtkAnnotatedCubeActor&) = delete;
};

#endif
