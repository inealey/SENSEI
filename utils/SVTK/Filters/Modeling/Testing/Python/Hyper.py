#!/usr/bin/env python
import os
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

SVTK_INTEGRATE_BOTH_DIRECTIONS = 2

#
# generate tensors
ptLoad = svtk.svtkPointLoad()
ptLoad.SetLoadValue(100.0)
ptLoad.SetSampleDimensions(20, 20, 20)
ptLoad.ComputeEffectiveStressOn()
ptLoad.SetModelBounds(-10, 10, -10, 10, -10, 10)

#
# If the current directory is writable, then test the writers
#
try:
    channel = open("wSP.svtk", "wb")
    channel.close()

    wSP = svtk.svtkDataSetWriter()
    wSP.SetInputConnection(ptLoad.GetOutputPort())
    wSP.SetFileName("wSP.svtk")
    wSP.SetTensorsName("pointload")
    wSP.SetScalarsName("effective_stress")
    wSP.Write()

    rSP = svtk.svtkDataSetReader()
    rSP.SetFileName("wSP.svtk")
    rSP.SetTensorsName("pointload")
    rSP.SetScalarsName("effective_stress")
    rSP.Update()

    input = rSP.GetOutput()

    # cleanup
    #
    try:
        os.remove("wSP.svtk")
    except OSError:
        pass

except IOError:
    print("Unable to test the writer/reader.")
    input = ptLoad.GetOutput()

# Generate hyperstreamlines
s1 = svtk.svtkHyperStreamline()
s1.SetInputData(input)
s1.SetStartPosition(9, 9, -9)
s1.IntegrateMinorEigenvector()
s1.SetMaximumPropagationDistance(18.0)
s1.SetIntegrationStepLength(0.1)
s1.SetStepLength(0.01)
s1.SetRadius(0.25)
s1.SetNumberOfSides(18)
s1.SetIntegrationDirection(SVTK_INTEGRATE_BOTH_DIRECTIONS)
s1.Update()

# Map hyperstreamlines
lut = svtk.svtkLogLookupTable()
lut.SetHueRange(.6667, 0.0)

s1Mapper = svtk.svtkPolyDataMapper()
s1Mapper.SetInputConnection(s1.GetOutputPort())
s1Mapper.SetLookupTable(lut)
# force update for scalar range
ptLoad.Update()
s1Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())

s1Actor = svtk.svtkActor()
s1Actor.SetMapper(s1Mapper)

s2 = svtk.svtkHyperStreamline()
s2.SetInputData(input)
s2.SetStartPosition(-9, -9, -9)
s2.IntegrateMinorEigenvector()
s2.SetMaximumPropagationDistance(18.0)
s2.SetIntegrationStepLength(0.1)
s2.SetStepLength(0.01)
s2.SetRadius(0.25)
s2.SetNumberOfSides(18)
s2.SetIntegrationDirection(SVTK_INTEGRATE_BOTH_DIRECTIONS)
s2.Update()

s2Mapper = svtk.svtkPolyDataMapper()
s2Mapper.SetInputConnection(s2.GetOutputPort())
s2Mapper.SetLookupTable(lut)
s2Mapper.SetScalarRange(input.GetScalarRange())

s2Actor = svtk.svtkActor()
s2Actor.SetMapper(s2Mapper)

s3 = svtk.svtkHyperStreamline()
s3.SetInputData(input)
s3.SetStartPosition(9, -9, -9)
s3.IntegrateMinorEigenvector()
s3.SetMaximumPropagationDistance(18.0)
s3.SetIntegrationStepLength(0.1)
s3.SetStepLength(0.01)
s3.SetRadius(0.25)
s3.SetNumberOfSides(18)
s3.SetIntegrationDirection(SVTK_INTEGRATE_BOTH_DIRECTIONS)
s3.Update()

s3Mapper = svtk.svtkPolyDataMapper()
s3Mapper.SetInputConnection(s3.GetOutputPort())
s3Mapper.SetLookupTable(lut)
s3Mapper.SetScalarRange(input.GetScalarRange())

s3Actor = svtk.svtkActor()
s3Actor.SetMapper(s3Mapper)

s4 = svtk.svtkHyperStreamline()
s4.SetInputData(input)
s4.SetStartPosition(-9, 9, -9)
s4.IntegrateMinorEigenvector()
s4.SetMaximumPropagationDistance(18.0)
s4.SetIntegrationStepLength(0.1)
s4.SetStepLength(0.01)
s4.SetRadius(0.25)
s4.SetNumberOfSides(18)
s4.SetIntegrationDirection(SVTK_INTEGRATE_BOTH_DIRECTIONS)
s4.Update()

s4Mapper = svtk.svtkPolyDataMapper()
s4Mapper.SetInputConnection(s4.GetOutputPort())
s4Mapper.SetLookupTable(lut)
s4Mapper.SetScalarRange(input.GetScalarRange())

s4Actor = svtk.svtkActor()
s4Actor.SetMapper(s4Mapper)

# plane for context
#
g = svtk.svtkImageDataGeometryFilter()
g.SetInputData(input)
g.SetExtent(0, 100, 0, 100, 0, 0)
g.Update()

# for scalar range
gm = svtk.svtkPolyDataMapper()
gm.SetInputConnection(g.GetOutputPort())
gm.SetScalarRange(g.GetOutput().GetScalarRange())

ga = svtk.svtkActor()
ga.SetMapper(gm)

# Create outline around data
#
outline = svtk.svtkOutlineFilter()
outline.SetInputData(input)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create cone indicating application of load
#
coneSrc = svtk.svtkConeSource()
coneSrc.SetRadius(.5)
coneSrc.SetHeight(2)

coneMap = svtk.svtkPolyDataMapper()
coneMap.SetInputConnection(coneSrc.GetOutputPort())

coneActor = svtk.svtkActor()
coneActor.SetMapper(coneMap)
coneActor.SetPosition(0, 0, 11)
coneActor.RotateY(90)
coneActor.GetProperty().SetColor(1, 0, 0)

camera = svtk.svtkCamera()
camera.SetFocalPoint(0.113766, -1.13665, -1.01919)
camera.SetPosition(-29.4886, -63.1488, 26.5807)
camera.SetViewAngle(24.4617)
camera.SetViewUp(0.17138, 0.331163, 0.927879)
camera.SetClippingRange(1, 100)

ren1.AddActor(s1Actor)
ren1.AddActor(s2Actor)
ren1.AddActor(s3Actor)
ren1.AddActor(s4Actor)
ren1.AddActor(outlineActor)
ren1.AddActor(coneActor)
ren1.AddActor(ga)

ren1.SetBackground(1.0, 1.0, 1.0)
ren1.SetActiveCamera(camera)

renWin.SetSize(300, 300)

renWin.Render()

iren.Initialize()
#iren.Start()
