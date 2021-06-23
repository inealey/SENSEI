#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create a rendering window and renderer
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.StereoCapableWindowOn()
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = svtk.svtkGenericEnSightReader()
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/elements6-bin.case")
reader.UpdateInformation()
reader.GetOutputInformation(0).Set(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 0.1)
geom = svtk.svtkGeometryFilter()
geom.SetInputConnection(reader.GetOutputPort())
mapper = svtk.svtkHierarchicalPolyDataMapper()
mapper.SetInputConnection(geom.GetOutputPort())
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUsePointFieldData()
mapper.ColorByArrayComponent("pointTensors",0)
mapper.SetScalarRange(0,300)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# assign our actor to the renderer
ren1.AddActor(actor)
# enable user interface interactor
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
