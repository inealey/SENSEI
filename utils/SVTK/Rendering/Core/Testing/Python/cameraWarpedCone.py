#!/usr/bin/env python

import svtk

# create a rendering window and renderer
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(300,300)

# create an actor and give it cone geometry
cone = svtk.svtkConeSource()
cone.SetResolution(8)
coneMapper = svtk.svtkPolyDataMapper()
coneMapper.SetInputConnection(cone.GetOutputPort())
coneActor = svtk.svtkActor()
coneActor.SetMapper(coneMapper)

# create a transform and distort the camera using it
mat = svtk.svtkMatrix4x4()
mat.SetElement(0,0,0.5)
mat.SetElement(0,1,0)
mat.SetElement(0,2,0)
mat.SetElement(0,3,0)
mat.SetElement(1,0,0)
mat.SetElement(1,1,1)
mat.SetElement(1,2,0)
mat.SetElement(1,3,0)
mat.SetElement(2,0,0)
mat.SetElement(2,1,0)
mat.SetElement(2,2,1)
mat.SetElement(2,3,0)
mat.SetElement(3,0,0)
mat.SetElement(3,1,0)
mat.SetElement(3,2,0)
mat.SetElement(3,3,1)

trans = svtk.svtkTransform()
trans.SetMatrix(mat)

# assign our actor to the renderer
ren.AddActor(coneActor)

ren.ResetCamera();
ren.GetActiveCamera().SetUserTransform(trans);

renWin.Render()
