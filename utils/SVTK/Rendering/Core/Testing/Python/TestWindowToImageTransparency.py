#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow and Renderer
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetAlphaBitPlanes(1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a default polygonal sphere
sphere = svtk.svtkSphereSource()
sphmapper = svtk.svtkPolyDataMapper()
sphmapper.SetInputConnection(sphere.GetOutputPort())

sphactor = svtk.svtkActor()
sphactor.SetMapper(sphmapper)

# Add the actors to the renderer, set the background to initial
# color (which is also transparent), set size.
ren1.AddActor(sphactor)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(256, 256)

# render first image
renWin.Render()
if 0 == renWin.GetAlphaBitPlanes():
    print("Failed to find a visual with alpha bit planes.")
    exit(0)
else:
    print("GetAlphaBitPlanes: " + str(renWin.GetAlphaBitPlanes()))

# create window to image filter, grabbing RGB and alpha
w2i = svtk.svtkWindowToImageFilter()
w2i.SetInput(renWin)
w2i.SetInputBufferTypeToRGBA()

# grab window
w2i.Update()

# copy the output
outputData = w2i.GetOutput().NewInstance()
outputData.DeepCopy(w2i.GetOutput())

# set up mappers and actors to display the image
im = svtk.svtkImageMapper()
im.SetColorWindow(255)
im.SetColorLevel(127.5)
im.SetInputData(outputData)

ia2 = svtk.svtkActor2D()
ia2.SetMapper(im)

# now, change the image (background is now green)
sphactor.SetScale(2, 2, 2)

ren1.SetBackground(0, 1, 0)
# add the image of the sphere (keeping the original sphere too)
ren1.AddActor(ia2)
ren1.SetViewport(0, 0, 1, 1)

# render result (the polygonal sphere appears behind a smaller image
# of itself).  Background of original image is transparent, so you
# can see through it back to the larger sphere and new background.
renWin.Render()

#iren.Start();
