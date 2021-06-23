#!/usr/bin/env python

import os
import svtk
from svtk.test import Testing

class SimpleGlyph:
    """A simple class used to test svtkTensorGlyph."""
    def __init__(self, reader):
        self.reader = reader

        sg = self.src_glyph = svtk.svtkSphereSource()
        sg.SetRadius(0.5)
        sg.SetCenter(0.5, 0.0, 0.0)

        g = self.glyph = svtk.svtkTensorGlyph()
        g.SetInputConnection(self.reader.GetOutputPort())
        g.SetSourceConnection(self.src_glyph.GetOutputPort())
        g.SetScaleFactor(0.25)

        # The normals are needed to generate the right colors and if
        # not used some of the glyphs are black.
        self.normals = svtk.svtkPolyDataNormals()
        self.normals.SetInputConnection(g.GetOutputPort())

        self.map = svtk.svtkPolyDataMapper()
        self.map.SetInputConnection(self.normals.GetOutputPort())

        self.act = svtk.svtkActor()
        self.act.SetMapper(self.map)

        # An outline.
        self.of = svtk.svtkOutlineFilter()
        self.of.SetInputConnection(self.reader.GetOutputPort())

        self.out_map = svtk.svtkPolyDataMapper()
        self.out_map.SetInputConnection(self.of.GetOutputPort())

        self.out_act = svtk.svtkActor()
        self.out_act.SetMapper(self.out_map)

    def GetActors(self):
        return self.act, self.out_act

    def Update(self):
        self.glyph.Update()

        s = self.glyph.GetOutput().GetPointData().GetScalars()
        if s:
            self.map.SetScalarRange(s.GetRange())

    def SetPosition(self, pos):
        self.act.SetPosition(pos)
        self.out_act.SetPosition(pos)


class TestTensorGlyph(Testing.svtkTest):
    def testGlyphs(self):
        '''Test if the glyphs are created nicely.'''
        reader = svtk.svtkDataSetReader()

        data_file = os.path.join(Testing.SVTK_DATA_ROOT, "Data", "tensors.svtk")

        reader.SetFileName(data_file)

        g1 = SimpleGlyph(reader)
        g1.glyph.ColorGlyphsOff()
        g1.Update()

        g2 = SimpleGlyph(reader)
        g2.glyph.ExtractEigenvaluesOff()
        g2.Update()
        g2.SetPosition((2.0, 0.0, 0.0))

        g3 = SimpleGlyph(reader)
        g3.glyph.SetColorModeToEigenvalues()
        g3.glyph.ThreeGlyphsOn()
        g3.Update()
        g3.SetPosition((0.0, 2.0, 0.0))

        g4 = SimpleGlyph(reader)
        g4.glyph.SetColorModeToEigenvalues()
        g4.glyph.ThreeGlyphsOn()
        g4.glyph.SymmetricOn()
        g4.Update()
        g4.SetPosition((2.0, 2.0, 0.0))

        # 6Components symmetric tensor
        g5 = SimpleGlyph(reader)
        g5.glyph.SetInputArrayToProcess(0, 0, 0, 0, "symTensors1")
        g5.SetPosition((4.0, 2.0, 0.0))
        g5.Update()

        ren = svtk.svtkRenderer()
        for i in (g1, g2, g3, g4, g5):
            for j in i.GetActors():
                ren.AddActor(j)

        ren.ResetCamera();

        cam = ren.GetActiveCamera()
        cam.Azimuth(-20)
        cam.Elevation(20)
        cam.Zoom(1.1)

        ren.SetBackground(0.5, 0.5, 0.5)

        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)
        renWin.Render()

        img_file = "TestTensorGlyph.png"
        Testing.compareImage(renWin, Testing.getAbsImagePath(img_file))
        Testing.interact()

if __name__ == "__main__":
    Testing.main([(TestTensorGlyph, 'test')])
