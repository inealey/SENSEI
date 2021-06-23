#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName("" + str(SVTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d.SetQFileName("" + str(SVTK_DATA_ROOT) + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
ps = svtk.svtkPlaneSource()
ps.SetXResolution(4)
ps.SetYResolution(4)
ps.SetOrigin(2,-2,26)
ps.SetPoint1(2,2,26)
ps.SetPoint2(2,-2,32)
psMapper = svtk.svtkPolyDataMapper()
psMapper.SetInputConnection(ps.GetOutputPort())
psActor = svtk.svtkActor()
psActor.SetMapper(psMapper)
psActor.GetProperty().SetRepresentationToWireframe()
rk4 = svtk.svtkRungeKutta4()
streamer = svtk.svtkStreamTracer()
streamer.SetInputData(output)
streamer.SetSourceData(ps.GetOutput())
streamer.SetMaximumPropagation(100)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetComputeVorticity(1)
streamer.SetIntegrator(rk4)
rf = svtk.svtkRibbonFilter()
rf.SetInputConnection(streamer.GetOutputPort())
rf.SetInputArrayToProcess(1, 0, 0, svtk.svtkDataObject.FIELD_ASSOCIATION_POINTS, "Normals")
rf.SetWidth(0.1)
rf.SetWidthFactor(5)
streamMapper = svtk.svtkPolyDataMapper()
streamMapper.SetInputConnection(rf.GetOutputPort())
streamMapper.SetScalarRange(output.GetScalarRange())
streamline = svtk.svtkActor()
streamline.SetMapper(streamMapper)
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(psActor)
ren1.AddActor(outlineActor)
ren1.AddActor(streamline)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
ren1.SetBackground(0.1,0.2,0.4)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(9.71821,0.458166,29.3999)
cam1.SetPosition(2.7439,-37.3196,38.7167)
cam1.SetViewUp(-0.16123,0.264271,0.950876)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# for testing
threshold = 15
iren.Start()
# --- end of script --
