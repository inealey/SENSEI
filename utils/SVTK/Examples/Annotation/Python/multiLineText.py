#!/usr/bin/env python

# This example demonstrates the use of multiline 2D text using
# svtkTextMappers.  It shows several justifications as well as
# single-line and multiple-line text inputs.

import svtk

font_size = 14

# Create the text mappers and the associated Actor2Ds.

# The font and text properties (except justification) are the same for
# each single line mapper. Let's create a common text property object
singleLineTextProp = svtk.svtkTextProperty()
singleLineTextProp.SetFontSize(font_size)
singleLineTextProp.SetFontFamilyToArial()
singleLineTextProp.BoldOff()
singleLineTextProp.ItalicOff()
singleLineTextProp.ShadowOff()

# The font and text properties (except justification) are the same for
# each multi line mapper. Let's create a common text property object
multiLineTextProp = svtk.svtkTextProperty()
multiLineTextProp.ShallowCopy(singleLineTextProp)
multiLineTextProp.BoldOn()
multiLineTextProp.ItalicOn()
multiLineTextProp.ShadowOn()
multiLineTextProp.SetLineSpacing(0.8)

# The text is on a single line and bottom-justified.
singleLineTextB = svtk.svtkTextMapper()
singleLineTextB.SetInput("Single line (bottom)")
tprop = singleLineTextB.GetTextProperty()
tprop.ShallowCopy(singleLineTextProp)
tprop.SetVerticalJustificationToBottom()
tprop.SetColor(1, 0, 0)
singleLineTextActorB = svtk.svtkActor2D()
singleLineTextActorB.SetMapper(singleLineTextB)
singleLineTextActorB.GetPositionCoordinate().SetCoordinateSystemToNormalizedDisplay()
singleLineTextActorB.GetPositionCoordinate().SetValue(0.05, 0.85)

# The text is on a single line and center-justified (vertical
# justification).
singleLineTextC = svtk.svtkTextMapper()
singleLineTextC.SetInput("Single line (centered)")
tprop = singleLineTextC.GetTextProperty()
tprop.ShallowCopy(singleLineTextProp)
tprop.SetVerticalJustificationToCentered()
tprop.SetColor(0, 1, 0)
singleLineTextActorC = svtk.svtkActor2D()
singleLineTextActorC.SetMapper(singleLineTextC)
singleLineTextActorC.GetPositionCoordinate().SetCoordinateSystemToNormalizedDisplay()
singleLineTextActorC.GetPositionCoordinate().SetValue(0.05, 0.75)

# The text is on a single line and top-justified.
singleLineTextT = svtk.svtkTextMapper()
singleLineTextT.SetInput("Single line (top)")
tprop = singleLineTextT.GetTextProperty()
tprop.ShallowCopy(singleLineTextProp)
tprop.SetVerticalJustificationToTop()
tprop.SetColor(0, 0, 1)
singleLineTextActorT = svtk.svtkActor2D()
singleLineTextActorT.SetMapper(singleLineTextT)
singleLineTextActorT.GetPositionCoordinate().SetCoordinateSystemToNormalizedDisplay()
singleLineTextActorT.GetPositionCoordinate().SetValue(0.05, 0.65)

# The text is on multiple lines and left- and top-justified.
textMapperL = svtk.svtkTextMapper()
textMapperL.SetInput("This is\nmulti-line\ntext output\n(left-top)")
tprop = textMapperL.GetTextProperty()
tprop.ShallowCopy(multiLineTextProp)
tprop.SetJustificationToLeft()
tprop.SetVerticalJustificationToTop()
tprop.SetColor(1, 0, 0)
textActorL = svtk.svtkActor2D()
textActorL.SetMapper(textMapperL)
textActorL.GetPositionCoordinate().SetCoordinateSystemToNormalizedDisplay()
textActorL.GetPositionCoordinate().SetValue(0.05, 0.5)

# The text is on multiple lines and center-justified (both horizontal and
# vertical).
textMapperC = svtk.svtkTextMapper()
textMapperC.SetInput("This is\nmulti-line\ntext output\n(centered)")
tprop = textMapperC.GetTextProperty()
tprop.ShallowCopy(multiLineTextProp)
tprop.SetJustificationToCentered()
tprop.SetVerticalJustificationToCentered()
tprop.SetColor(0, 1, 0)
textActorC = svtk.svtkActor2D()
textActorC.SetMapper(textMapperC)
textActorC.GetPositionCoordinate().SetCoordinateSystemToNormalizedDisplay()
textActorC.GetPositionCoordinate().SetValue(0.5, 0.5)

# The text is on multiple lines and right- and bottom-justified.
textMapperR = svtk.svtkTextMapper()
textMapperR.SetInput("This is\nmulti-line\ntext output\n(right-bottom)")
tprop = textMapperR.GetTextProperty()
tprop.ShallowCopy(multiLineTextProp)
tprop.SetJustificationToRight()
tprop.SetVerticalJustificationToBottom()
tprop.SetColor(0, 0, 1)
textActorR = svtk.svtkActor2D()
textActorR.SetMapper(textMapperR)
textActorR.GetPositionCoordinate().SetCoordinateSystemToNormalizedDisplay()
textActorR.GetPositionCoordinate().SetValue(0.95, 0.5)

# Draw the grid to demonstrate the placement of the text.

# Set up the necessary points.
Pts = svtk.svtkPoints()
Pts.InsertNextPoint(0.05, 0.0, 0.0)
Pts.InsertNextPoint(0.05, 1.0, 0.0)
Pts.InsertNextPoint(0.5, 0.0, 0.0)
Pts.InsertNextPoint(0.5, 1.0, 0.0)
Pts.InsertNextPoint(0.95, 0.0, 0.0)
Pts.InsertNextPoint(0.95, 1.0, 0.0)
Pts.InsertNextPoint(0.0, 0.5, 0.0)
Pts.InsertNextPoint(1.0, 0.5, 0.0)
Pts.InsertNextPoint(0.00, 0.85, 0.0)
Pts.InsertNextPoint(0.50, 0.85, 0.0)
Pts.InsertNextPoint(0.00, 0.75, 0.0)
Pts.InsertNextPoint(0.50, 0.75, 0.0)
Pts.InsertNextPoint(0.00, 0.65, 0.0)
Pts.InsertNextPoint(0.50, 0.65, 0.0)
# Set up the lines that use these points.
Lines = svtk.svtkCellArray()
Lines.InsertNextCell(2)
Lines.InsertCellPoint(0)
Lines.InsertCellPoint(1)
Lines.InsertNextCell(2)
Lines.InsertCellPoint(2)
Lines.InsertCellPoint(3)
Lines.InsertNextCell(2)
Lines.InsertCellPoint(4)
Lines.InsertCellPoint(5)
Lines.InsertNextCell(2)
Lines.InsertCellPoint(6)
Lines.InsertCellPoint(7)
Lines.InsertNextCell(2)
Lines.InsertCellPoint(8)
Lines.InsertCellPoint(9)
Lines.InsertNextCell(2)
Lines.InsertCellPoint(10)
Lines.InsertCellPoint(11)
Lines.InsertNextCell(2)
Lines.InsertCellPoint(12)
Lines.InsertCellPoint(13)
# Create a grid that uses these points and lines.
Grid = svtk.svtkPolyData()
Grid.SetPoints(Pts)
Grid.SetLines(Lines)
# Set up the coordinate system.
normCoords = svtk.svtkCoordinate()
normCoords.SetCoordinateSystemToNormalizedViewport()

# Set up the mapper and actor (2D) for the grid.
mapper = svtk.svtkPolyDataMapper2D()
mapper.SetInputData(Grid)
mapper.SetTransformCoordinate(normCoords)
gridActor = svtk.svtkActor2D()
gridActor.SetMapper(mapper)
gridActor.GetProperty().SetColor(0.1, 0.1, 0.1)

# Create the Renderer, RenderWindow, and RenderWindowInteractor
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer; set the background and size; zoom in
# closer to the image; render
ren.AddActor2D(textActorL)
ren.AddActor2D(textActorC)
ren.AddActor2D(textActorR)
ren.AddActor2D(singleLineTextActorB)
ren.AddActor2D(singleLineTextActorC)
ren.AddActor2D(singleLineTextActorT)
ren.AddActor2D(gridActor)

ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 300)
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
