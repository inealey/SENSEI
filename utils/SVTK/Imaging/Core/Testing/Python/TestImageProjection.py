#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# this script tests svtkImageSlab with various axes permutations,
# in order to cover a nasty set of "if" statements that check
# the intersections of the raster lines with the input bounding box.
# Image pipeline
reader = svtk.svtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
slab1 = svtk.svtkImageSlab()
slab1.SetInputConnection(reader.GetOutputPort())
slab1.SetOperationToMean()
slab1.TrapezoidIntegrationOn()
slab1.SetOrientationToZ()
slab2 = svtk.svtkImageSlab()
slab2.SetInputConnection(reader.GetOutputPort())
slab2.SetOperationToMax()
slab2.MultiSliceOutputOff()
slab2.SetOutputScalarTypeToInputScalarType()
slab3 = svtk.svtkImageSlab()
slab3.SetInputConnection(reader.GetOutputPort())
slab3.SetOperationToSum()
slab3.SetOrientationToX()
slab3.MultiSliceOutputOn()
slab3.SetOutputScalarTypeToDouble()
reslice3 = svtk.svtkImageReslice()
reslice3.SetInputConnection(slab3.GetOutputPort())
reslice3.SetResliceAxesDirectionCosines([0,1,0,0,0,-1,1,0,0])
reslice3.SetOutputSpacing(3.2,3.2,3.2)
reslice3.SetOutputExtent(0,74,0,74,0,0)
slab4 = svtk.svtkImageSlab()
slab4.SetInputConnection(reader.GetOutputPort())
slab4.SetOperationToMax()
slab4.SetOrientation(0)
slab4.MultiSliceOutputOn()
slab4.SetOutputScalarTypeToFloat()
reslice4 = svtk.svtkImageReslice()
reslice4.SetInputConnection(slab4.GetOutputPort())
reslice4.SetResliceAxesDirectionCosines([0,1,0,0,0,-1,1,0,0])
reslice4.SetOutputSpacing(3.2,3.2,3.2)
reslice4.SetOutputExtent(0,74,0,74,0,0)
slab5 = svtk.svtkImageSlab()
slab5.SetInputConnection(reader.GetOutputPort())
slab5.SetOperationToMean()
slab5.SetOrientationToY()
slab5.MultiSliceOutputOn()
reslice5 = svtk.svtkImageReslice()
reslice5.SetInputConnection(slab5.GetOutputPort())
reslice5.SetResliceAxesDirectionCosines([1,0,0,0,0,-1,0,1,0])
reslice5.SetOutputSpacing(3.2,3.2,3.2)
reslice5.SetOutputExtent(0,74,0,74,0,0)
slab6 = svtk.svtkImageSlab()
slab6.SetInputConnection(reader.GetOutputPort())
slab6.SetOperationToMax()
slab6.SetOrientation(1)
slab6.MultiSliceOutputOn()
reslice6 = svtk.svtkImageReslice()
reslice6.SetInputConnection(slab6.GetOutputPort())
reslice6.SetResliceAxesDirectionCosines([1,0,0,0,0,-1,0,1,0])
reslice6.SetOutputSpacing(3.2,3.2,3.2)
reslice6.SetOutputExtent(0,74,0,74,0,0)
mapper1 = svtk.svtkImageMapper()
mapper1.SetInputConnection(slab1.GetOutputPort())
mapper1.SetColorWindow(2000)
mapper1.SetColorLevel(1000)
mapper1.SetZSlice(0)
mapper2 = svtk.svtkImageMapper()
mapper2.SetInputConnection(slab2.GetOutputPort())
mapper2.SetColorWindow(2000)
mapper2.SetColorLevel(1000)
mapper2.SetZSlice(0)
mapper3 = svtk.svtkImageMapper()
mapper3.SetInputConnection(reslice3.GetOutputPort())
mapper3.SetColorWindow(128000)
mapper3.SetColorLevel(64000)
mapper3.SetZSlice(0)
mapper4 = svtk.svtkImageMapper()
mapper4.SetInputConnection(reslice4.GetOutputPort())
mapper4.SetColorWindow(2000)
mapper4.SetColorLevel(1000)
mapper4.SetZSlice(0)
mapper5 = svtk.svtkImageMapper()
mapper5.SetInputConnection(reslice5.GetOutputPort())
mapper5.SetColorWindow(2000)
mapper5.SetColorLevel(1000)
mapper5.SetZSlice(0)
mapper6 = svtk.svtkImageMapper()
mapper6.SetInputConnection(reslice6.GetOutputPort())
mapper6.SetColorWindow(2000)
mapper6.SetColorLevel(1000)
mapper6.SetZSlice(0)
actor1 = svtk.svtkActor2D()
actor1.SetMapper(mapper1)
actor2 = svtk.svtkActor2D()
actor2.SetMapper(mapper2)
actor3 = svtk.svtkActor2D()
actor3.SetMapper(mapper3)
actor4 = svtk.svtkActor2D()
actor4.SetMapper(mapper4)
actor5 = svtk.svtkActor2D()
actor5.SetMapper(mapper5)
actor6 = svtk.svtkActor2D()
actor6.SetMapper(mapper6)
imager1 = svtk.svtkRenderer()
imager1.AddActor2D(actor1)
imager1.SetViewport(0.0,0.0,0.3333,0.5)
imager2 = svtk.svtkRenderer()
imager2.AddActor2D(actor2)
imager2.SetViewport(0.0,0.5,0.3333,1.0)
imager3 = svtk.svtkRenderer()
imager3.AddActor2D(actor3)
imager3.SetViewport(0.3333,0.0,0.6667,0.5)
imager4 = svtk.svtkRenderer()
imager4.AddActor2D(actor4)
imager4.SetViewport(0.3333,0.5,0.6667,1.0)
imager5 = svtk.svtkRenderer()
imager5.AddActor2D(actor5)
imager5.SetViewport(0.6667,0.0,1.0,0.5)
imager6 = svtk.svtkRenderer()
imager6.AddActor2D(actor6)
imager6.SetViewport(0.6667,0.5,1.0,1.0)
imgWin = svtk.svtkRenderWindow()
imgWin.AddRenderer(imager1)
imgWin.AddRenderer(imager2)
imgWin.AddRenderer(imager3)
imgWin.AddRenderer(imager4)
imgWin.AddRenderer(imager5)
imgWin.AddRenderer(imager6)
imgWin.SetSize(230,150)
imgWin.Render()
# --- end of script --
