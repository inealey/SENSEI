#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    svtk.svtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Demonstrate how to use structured grid blanking with an image. There are two
# techniques demonstrated: one uses an image to perform the blanking;
# the other uses scalar values to do the same thing. Both images should
# be identical.
#
# create pipeline - start by extracting a single plane from the grid
#
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

plane = svtk.svtkExtractGrid()
plane.SetInputData(output)
plane.SetVOI(0, 57, 0, 33, 0, 0)
plane.Update()

# Create some data to use for the (image) blanking
#
blankImage = svtk.svtkImageData()

# svtkType.h has definitions for svtk datatypes SVTK_INT, SVTK_FLOAT, etc. that
# don't get wrapped in Python.
SVTK_UNSIGNED_CHAR = 3

blankImage.SetDimensions(57, 33, 1)
blankImage.AllocateScalars(SVTK_UNSIGNED_CHAR, 1)
blankImage.GetPointData().GetScalars().SetName("blankScalars")

blanking = blankImage.GetPointData().GetScalars()
numBlanks = 57 * 33
i = 0
while i < numBlanks:
    blanking.SetComponent(i, 0, svtk.svtkDataSetAttributes.HIDDENPOINT)
    i += 1

# Manually blank out areas corresponding to dilution holes
blanking.SetComponent(318, 0, 0)
blanking.SetComponent(945, 0, 0)
blanking.SetComponent(1572, 0, 0)
blanking.SetComponent(641, 0, 0)
blanking.SetComponent(1553, 0, 0)

# The first blanking technique uses the image to set the blanking values
#
blankIt = svtk.svtkBlankStructuredGridWithImage()
blankIt.SetInputConnection(plane.GetOutputPort())
blankIt.SetBlankingInputData(blankImage)

blankedPlane = svtk.svtkStructuredGridGeometryFilter()
blankedPlane.SetInputConnection(blankIt.GetOutputPort())
blankedPlane.SetExtent(0, 100, 0, 100, 0, 0)

planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(blankedPlane.GetOutputPort())
planeMapper.SetScalarRange(0.197813, 0.710419)

planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)

# The second blanking technique uses grid data values to create the blanking.
# Here we borrow the image data and threshold on that.
#
anotherGrid = svtk.svtkStructuredGrid()
anotherGrid.CopyStructure(plane.GetOutput())
anotherGrid.GetPointData().SetScalars(blankImage.GetPointData().GetScalars())

blankGrid = svtk.svtkBlankStructuredGrid()
blankGrid.SetInputData(anotherGrid)
blankGrid.SetArrayName("blankScalars")
blankGrid.SetMinBlankingValue(-0.5)
blankGrid.SetMaxBlankingValue(0.5)

blankedPlane2 = svtk.svtkStructuredGridGeometryFilter()
blankedPlane2.SetInputConnection(blankGrid.GetOutputPort())
blankedPlane2.SetExtent(0, 100, 0, 100, 0, 0)

planeMapper2 = svtk.svtkPolyDataMapper()
planeMapper2.SetInputConnection(blankedPlane2.GetOutputPort())
planeMapper2.SetScalarRange(0.197813, 0.710419)

planeActor2 = svtk.svtkActor()
planeActor2.SetMapper(planeMapper2)

# An outline around the data
#
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(GetRGBColor('black'))

outlineMapper2 = svtk.svtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline.GetOutputPort())

outlineActor2 = svtk.svtkActor()
outlineActor2.SetMapper(outlineMapper2)
outlineActor2.GetProperty().SetColor(GetRGBColor('black'))

# create planes
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0, 0, 0.5, 1)
ren2 = svtk.svtkRenderer()
ren2.SetViewport(0.5, 0, 1, 1)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(planeActor)

ren2.AddActor(outlineActor2)
ren2.AddActor(planeActor2)

ren1.SetBackground(1, 1, 1)
ren2.SetBackground(1, 1, 1)

renWin.SetSize(500, 250)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(8.88908, 0.595038, 29.3342)
cam1.SetPosition(-12.3332, 31.7479, 41.2387)
cam1.SetViewUp(0.060772, -0.319905, 0.945498)

ren2.SetActiveCamera(ren1.GetActiveCamera())

# render the image
#
renWin.Render()

iren.Initialize()
#iren.Start()
