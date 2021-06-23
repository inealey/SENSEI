#!/usr/bin/env python

# This example demonstrates the subsampling of a structured grid.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read some structured data.
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# Here we subsample the grid. The SetVOI method requires six values
# specifying (imin,imax, jmin,jmax, kmin,kmax) extents. In this
# example we extracting a plane. Note that the VOI is clamped to zero
# (min) and the maximum i-j-k value; that way we can use the
# -1000,1000 specification and be sure the values are clamped. The
# SampleRate specifies that we take every point in the i-direction;
# every other point in the j-direction; and every third point in the
# k-direction. IncludeBoundaryOn makes sure that we get the boundary
# points even if the SampleRate does not coincident with the boundary.
extract = svtk.svtkExtractGrid()
extract.SetInputData(pl3d_output)
extract.SetVOI(30, 30, -1000, 1000, -1000, 1000)
extract.SetSampleRate(1, 2, 3)
extract.IncludeBoundaryOn()
mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(extract.GetOutputPort())
mapper.SetScalarRange(.18, .7)
actor = svtk.svtkActor()
actor.SetMapper(mapper)

outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Add the usual rendering stuff.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(actor)

ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 180)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(2.64586, 47.905)
cam1.SetFocalPoint(8.931, 0.358127, 31.3526)
cam1.SetPosition(29.7111, -0.688615, 37.1495)
cam1.SetViewUp(-0.268328, 0.00801595, 0.963294)

iren.Initialize()
renWin.Render()
iren.Start()
