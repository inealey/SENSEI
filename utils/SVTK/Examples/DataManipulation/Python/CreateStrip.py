#!/usr/bin/env python

# This script shows how to manually create a svtkPolyData with a
# triangle strip.

import svtk

# First we'll create some points.
points = svtk.svtkPoints()
points.InsertPoint(0, 0.0, 0.0, 0.0)
points.InsertPoint(1, 0.0, 1.0, 0.0)
points.InsertPoint(2, 1.0, 0.0, 0.0)
points.InsertPoint(3, 1.0, 1.0, 0.0)
points.InsertPoint(4, 2.0, 0.0, 0.0)
points.InsertPoint(5, 2.0, 1.0, 0.0)
points.InsertPoint(6, 3.0, 0.0, 0.0)
points.InsertPoint(7, 3.0, 1.0, 0.0)

# The cell array can be thought of as a connectivity list.  Here we
# specify the number of points followed by that number of point
# ids. This can be repeated as many times as there are primitives in
# the list.
strips = svtk.svtkCellArray()
strips.InsertNextCell(8) # number of points
strips.InsertCellPoint(0)
strips.InsertCellPoint(1)
strips.InsertCellPoint(2)
strips.InsertCellPoint(3)
strips.InsertCellPoint(4)
strips.InsertCellPoint(5)
strips.InsertCellPoint(6)
strips.InsertCellPoint(7)
profile = svtk.svtkPolyData()
profile.SetPoints(points)
profile.SetStrips(strips)

map = svtk.svtkPolyDataMapper()
map.SetInputData(profile)

strip = svtk.svtkActor()
strip.SetMapper(map)
strip.GetProperty().SetColor(0.3800, 0.7000, 0.1600)

# Create the usual rendering stuff.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(strip)

ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 250)

iren.Initialize()
renWin.Render()
iren.Start()
