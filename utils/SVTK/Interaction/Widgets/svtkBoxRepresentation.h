/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoxRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBoxRepresentation
 * @brief   a class defining the representation for the svtkBoxWidget2
 *
 * This class is a concrete representation for the svtkBoxWidget2. It
 * represents a box with seven handles: one on each of the six faces, plus a
 * center handle. Through interaction with the widget, the box
 * representation can be arbitrarily positioned in the 3D space.
 *
 * To use this representation, you normally use the PlaceWidget() method
 * to position the widget at a specified region in space.
 *
 * @warning
 * This class, and svtkBoxWidget2, are second generation SVTK
 * widgets. An earlier version of this functionality was defined in the
 * class svtkBoxWidget.
 *
 * @sa
 * svtkBoxWidget2 svtkBoxWidget
 */

#ifndef svtkBoxRepresentation_h
#define svtkBoxRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkPolyDataMapper;
class svtkLineSource;
class svtkSphereSource;
class svtkCellPicker;
class svtkProperty;
class svtkPolyData;
class svtkPoints;
class svtkPolyDataAlgorithm;
class svtkPointHandleRepresentation3D;
class svtkTransform;
class svtkPlane;
class svtkPlanes;
class svtkBox;
class svtkDoubleArray;
class svtkMatrix4x4;

class SVTKINTERACTIONWIDGETS_EXPORT svtkBoxRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkBoxRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkBoxRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Get the planes describing the implicit function defined by the box
   * widget. The user must provide the instance of the class svtkPlanes. Note
   * that svtkPlanes is a subclass of svtkImplicitFunction, meaning that it can
   * be used by a variety of filters to perform clipping, cutting, and
   * selection of data.  (The direction of the normals of the planes can be
   * reversed enabling the InsideOut flag.)
   */
  void GetPlanes(svtkPlanes* planes);

  // Get the underlying planes used by this rep
  // this can be used as a cropping planes in svtkMapper
  svtkPlane* GetUnderlyingPlane(int i) { return this->Planes[i]; }

  //@{
  /**
   * Set/Get the InsideOut flag. This data member is used in conjunction
   * with the GetPlanes() method. When off, the normals point out of the
   * box. When on, the normals point into the hexahedron.  InsideOut is off
   * by default.
   */
  svtkSetMacro(InsideOut, svtkTypeBool);
  svtkGetMacro(InsideOut, svtkTypeBool);
  svtkBooleanMacro(InsideOut, svtkTypeBool);
  //@}

  /**
   * Retrieve a linear transform characterizing the transformation of the
   * box. Note that the transformation is relative to where PlaceWidget()
   * was initially called. This method modifies the transform provided. The
   * transform can be used to control the position of svtkProp3D's, as well as
   * other transformation operations (e.g., svtkTransformPolyData).
   */
  virtual void GetTransform(svtkTransform* t);

  /**
   * Set the position, scale and orientation of the box widget using the
   * transform specified. Note that the transformation is relative to
   * where PlaceWidget() was initially called (i.e., the original bounding
   * box).
   */
  virtual void SetTransform(svtkTransform* t);

  /**
   * Grab the polydata (including points) that define the box widget. The
   * polydata consists of 6 quadrilateral faces and 15 points. The first
   * eight points define the eight corner vertices; the next six define the
   * -x,+x, -y,+y, -z,+z face points; and the final point (the 15th out of 15
   * points) defines the center of the box. These point values are guaranteed
   * to be up-to-date when either the widget's corresponding InteractionEvent
   * or EndInteractionEvent events are invoked. The user provides the
   * svtkPolyData and the points and cells are added to it.
   */
  void GetPolyData(svtkPolyData* pd);

  //@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles, when selected or normal, can be
   * specified.
   */
  svtkGetObjectMacro(HandleProperty, svtkProperty);
  svtkGetObjectMacro(SelectedHandleProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the face properties (the faces of the box). The
   * properties of the face when selected and normal can be
   * set.
   */
  svtkGetObjectMacro(FaceProperty, svtkProperty);
  svtkGetObjectMacro(SelectedFaceProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the outline properties (the outline of the box). The
   * properties of the outline when selected and normal can be
   * set.
   */
  svtkGetObjectMacro(OutlineProperty, svtkProperty);
  svtkGetObjectMacro(SelectedOutlineProperty, svtkProperty);
  //@}

  //@{
  /**
   * Control the representation of the outline. This flag enables
   * face wires. By default face wires are off.
   */
  void SetOutlineFaceWires(int);
  svtkGetMacro(OutlineFaceWires, int);
  void OutlineFaceWiresOn() { this->SetOutlineFaceWires(1); }
  void OutlineFaceWiresOff() { this->SetOutlineFaceWires(0); }
  //@}

  //@{
  /**
   * Control the representation of the outline. This flag enables
   * the cursor lines running between the handles. By default cursor
   * wires are on.
   */
  void SetOutlineCursorWires(int);
  svtkGetMacro(OutlineCursorWires, int);
  void OutlineCursorWiresOn() { this->SetOutlineCursorWires(1); }
  void OutlineCursorWiresOff() { this->SetOutlineCursorWires(0); }
  //@}

  //@{
  /**
   * Switches handles (the spheres) on or off by manipulating the underlying
   * actor visibility.
   */
  virtual void HandlesOn();
  virtual void HandlesOff();
  //@}

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void StartComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  int ComputeComplexInteractionState(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  void EndComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  //@}

  //@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  // Used to manage the state of the widget
  enum
  {
    Outside = 0,
    MoveF0,
    MoveF1,
    MoveF2,
    MoveF3,
    MoveF4,
    MoveF5,
    Translating,
    Rotating,
    Scaling
  };

  /**
   * The interaction state may be set from a widget (e.g., svtkBoxWidget2) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  void SetInteractionState(int state);

  //@{
  /**
   * In two plane mode only the X planes are shown
   * this is useful for defining thick slabs
   */
  svtkGetMacro(TwoPlaneMode, bool);
  void SetTwoPlaneMode(bool);
  //@}

  //@{
  /**
   * For complex events should we snap orientations to
   * be aligned with the x y z axes
   */
  svtkGetMacro(SnapToAxes, bool);
  svtkSetMacro(SnapToAxes, bool);
  //@}

  //@{
  /**
   * For complex events should we snap orientations to
   * be aligned with the x y z axes
   */
  void StepForward();
  void StepBackward();
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  //@{
  /**
   * Gets/Sets the constraint axis for translations. Returns Axis::NONE
   * if none.
   **/
  svtkGetMacro(TranslationAxis, int);
  svtkSetClampMacro(TranslationAxis, int, -1, 2);
  //@}

  //@{
  /**
   * Toggles constraint translation axis on/off.
   */
  void SetXTranslationAxisOn() { this->TranslationAxis = Axis::XAxis; }
  void SetYTranslationAxisOn() { this->TranslationAxis = Axis::YAxis; }
  void SetZTranslationAxisOn() { this->TranslationAxis = Axis::ZAxis; }
  void SetTranslationAxisOff() { this->TranslationAxis = Axis::NONE; }
  //@}

  //@{
  /**
   * Returns true if ContrainedAxis
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }
  //@}

protected:
  svtkBoxRepresentation();
  ~svtkBoxRepresentation() override;

  // Manage how the representation appears
  double LastEventPosition[3];
  double LastEventOrientation[4];
  double StartEventOrientation[4];
  double SnappedEventOrientations[3][4];
  bool SnappedOrientation[3];
  bool SnapToAxes;

  bool TwoPlaneMode;

  // Constraint axis translation
  int TranslationAxis;

  // the hexahedron (6 faces)
  svtkActor* HexActor;
  svtkPolyDataMapper* HexMapper;
  svtkPolyData* HexPolyData;
  svtkPoints* Points; // used by others as well
  double N[6][3];    // the normals of the faces

  // A face of the hexahedron
  svtkActor* HexFace;
  svtkPolyDataMapper* HexFaceMapper;
  svtkPolyData* HexFacePolyData;

  // glyphs representing hot spots (e.g., handles)
  svtkActor** Handle;
  svtkPolyDataMapper** HandleMapper;
  svtkSphereSource** HandleGeometry;
  virtual void PositionHandles();
  int HighlightHandle(svtkProp* prop); // returns cell id
  void HighlightFace(int cellId);
  void HighlightOutline(int highlight);
  virtual void ComputeNormals();
  virtual void SizeHandles();

  // wireframe outline
  svtkActor* HexOutline;
  svtkPolyDataMapper* OutlineMapper;
  svtkPolyData* OutlinePolyData;

  // Do the picking
  svtkCellPicker* HandlePicker;
  svtkCellPicker* HexPicker;
  svtkActor* CurrentHandle;
  int CurrentHexFace;
  svtkCellPicker* LastPicker;

  // Transform the hexahedral points (used for rotations)
  svtkTransform* Transform;

  // Support GetBounds() method
  svtkBox* BoundingBox;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* HandleProperty;
  svtkProperty* SelectedHandleProperty;
  svtkProperty* FaceProperty;
  svtkProperty* SelectedFaceProperty;
  svtkProperty* OutlineProperty;
  svtkProperty* SelectedOutlineProperty;
  virtual void CreateDefaultProperties();

  // Control the orientation of the normals
  svtkTypeBool InsideOut;
  int OutlineFaceWires;
  int OutlineCursorWires;
  void GenerateOutline();

  // Helper methods
  virtual void Translate(const double* p1, const double* p2);
  virtual void Scale(const double* p1, const double* p2, int X, int Y);
  virtual void Rotate(int X, int Y, const double* p1, const double* p2, const double* vpn);
  void MovePlusXFace(const double* p1, const double* p2);
  void MoveMinusXFace(const double* p1, const double* p2);
  void MovePlusYFace(const double* p1, const double* p2);
  void MoveMinusYFace(const double* p1, const double* p2);
  void MovePlusZFace(const double* p1, const double* p2);
  void MoveMinusZFace(const double* p1, const double* p2);
  void UpdatePose(const double* p1, const double* d1, const double* p2, const double* d2);

  // Internal ivars for performance
  svtkPoints* PlanePoints;
  svtkDoubleArray* PlaneNormals;
  svtkMatrix4x4* Matrix;

  // The actual planes which are being manipulated
  svtkPlane* Planes[6];

  //"dir" is the direction in which the face can be moved i.e. the axis passing
  // through the center
  void MoveFace(const double* p1, const double* p2, const double* dir, double* x1, double* x2,
    double* x3, double* x4, double* x5);
  // Helper method to obtain the direction in which the face is to be moved.
  // Handles special cases where some of the scale factors are 0.
  void GetDirection(const double Nx[3], const double Ny[3], const double Nz[3], double dir[3]);

private:
  svtkBoxRepresentation(const svtkBoxRepresentation&) = delete;
  void operator=(const svtkBoxRepresentation&) = delete;
};

#endif
