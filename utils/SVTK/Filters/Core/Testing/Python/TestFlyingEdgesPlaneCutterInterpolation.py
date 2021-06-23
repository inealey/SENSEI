#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

res = 100

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = svtk.svtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.ComputeNormalsOff()
sample.Update()

# Now create some new attributes to interpolate
cyl = svtk.svtkCylinder()
cyl.SetRadius(0.1)
cyl.SetAxis(1,1,1)

attr = svtk.svtkSampleImplicitFunctionFilter()
attr.SetInputConnection(sample.GetOutputPort())
attr.SetImplicitFunction(cyl)
attr.ComputeGradientsOn()
attr.Update()

# The cut plane
plane = svtk.svtkPlane()
plane.SetOrigin(-.2,-.2,-.2)
plane.SetNormal(1,1,1)

# Perform the cutting on named scalars
cut = svtk.svtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(attr.GetOutputPort())
cut.SetInputArrayToProcess(0, 0, 0, svtk.svtkDataObject.FIELD_ASSOCIATION_POINTS, "scalars")
cut.SetPlane(plane)
cut.ComputeNormalsOff()
cut.InterpolateAttributesOn()

# Time the execution of the filter
timer = svtk.svtkExecutionTimer()
timer.SetFilter(cut)
cut.Update()
CG = timer.GetElapsedWallClockTime()
print ("Cut and interpolate volume:", CG)

cutMapper = svtk.svtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())
cutMapper.SetScalarModeToUsePointFieldData()
cutMapper.SelectColorArray("Implicit scalars")

cutActor = svtk.svtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(1,1,1)
cutActor.GetProperty().SetOpacity(1)

# Look at the vectors
ranPts = svtk.svtkMaskPoints()
ranPts.SetOnRatio(25)
ranPts.SetInputConnection(cut.GetOutputPort())

hhog = svtk.svtkHedgeHog()
hhog.SetInputConnection(ranPts.GetOutputPort())
hhog.SetVectorModeToUseVector()
hhog.SetScaleFactor(0.05)

hhogMapper = svtk.svtkPolyDataMapper()
hhogMapper.SetInputConnection(hhog.GetOutputPort())

hhogActor = svtk.svtkActor()
hhogActor.SetMapper(hhogMapper)
hhogActor.GetProperty().SetColor(1,1,1)
hhogActor.GetProperty().SetOpacity(1)

# Outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(cutActor)
ren1.AddActor(hhogActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
