/*=========================================================================
  Program:   Visualization Toolkit
  Module:    svtkOBJImporter.cxx
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "svtkOBJImporter.h"

#include "svtkActor.h"
#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtksys/SystemTools.hxx"

#include "svtkOBJImporterInternals.h"
#include <cctype>
#include <cstdio>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>

svtkStandardNewMacro(svtkOBJImporter);
svtkStandardNewMacro(svtkOBJPolyDataProcessor);

//----------------------------------------------------------------------------
svtkOBJImporter::svtkOBJImporter()
{
  this->Impl = svtkSmartPointer<svtkOBJPolyDataProcessor>::New();
}

//----------------------------------------------------------------------------
svtkOBJImporter::~svtkOBJImporter() = default;

int CanReadFile(svtkObject* that, const std::string& fname)
{
  FILE* fileFD = svtksys::SystemTools::Fopen(fname, "rb");
  if (fileFD == nullptr)
  {
    svtkErrorWithObjectMacro(that, << "Unable to open file: " << fname.c_str());
    return 0;
  }
  fclose(fileFD);
  return 1;
}

int svtkOBJImporter::ImportBegin()
{
  if (!CanReadFile(this, this->GetFileName()))
  {
    return 0;
  }
  if (!std::string(GetFileNameMTL()).empty() && !CanReadFile(this, this->GetFileNameMTL()))
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkOBJImporter::ImportEnd()
{
  svtkDebugMacro("Done with " << this->GetClassName() << "::" << __FUNCTION__);
}

//----------------------------------------------------------------------------
void svtkOBJImporter::ReadData()
{
  this->Impl->Update();
  if (Impl->GetSuccessParsingFiles())
  {
    bindTexturedPolydataToRenderWindow(this->RenderWindow, this->Renderer, Impl);
  }
}

//----------------------------------------------------------------------------
void svtkOBJImporter::PrintSelf(std::ostream& os, svtkIndent indent)
{
  svtkImporter::PrintSelf(os, indent);
}

void svtkOBJImporter::SetFileName(const char* arg)
{
  this->Impl->SetFileName(arg);
}

void svtkOBJImporter::SetFileNameMTL(const char* arg)
{
  this->Impl->SetMTLfileName(arg);
}

void svtkOBJImporter::SetTexturePath(const char* path)
{
  return this->Impl->SetTexturePath(path);
}

const char* svtkOBJImporter::GetFileName() const
{
  return this->Impl->GetFileName().data();
}

const char* svtkOBJImporter::GetFileNameMTL() const
{
  return this->Impl->GetMTLFileName().data();
}

const char* svtkOBJImporter::GetTexturePath() const
{
  return this->Impl->GetTexturePath().data();
}

//----------------------------------------------------------------------------
std::string svtkOBJImporter::GetOutputsDescription()
{
  std::stringstream ss;
  for (int i = 0; i < this->Impl->GetNumberOfOutputs(); i++)
  {
    ss << this->GetOutputDescription(i) << std::endl;
  }
  return ss.str();
}

//----------------------------------------------------------------------------
std::string svtkOBJImporter::GetOutputDescription(int idx)
{
  svtkOBJImportedMaterial* mtl = this->Impl->GetMaterial(idx);
  std::stringstream ss;
  ss << "data output " << idx;
  if (mtl)
  {
    ss << " with material named " << mtl->name << " texture file "
       << (mtl->texture_filename[0] == '\0' ? "none" : mtl->texture_filename) << " diffuse color ("
       << mtl->diff[0] << ", " << mtl->diff[1] << ", " << mtl->diff[2] << ")"
       << " ambient color (" << mtl->amb[0] << ", " << mtl->amb[1] << ", " << mtl->amb[2] << ")"
       << " specular color (" << mtl->spec[0] << ", " << mtl->spec[1] << ", " << mtl->spec[2] << ")"
       << " specular power " << mtl->specularPower << " opacity " << mtl->trans;
  }
  else
  {
    ss << " with no material";
  }

  return ss.str();
}

///////////////////////////////////////////

struct svtkOBJImportedPolyDataWithMaterial
{
  ~svtkOBJImportedPolyDataWithMaterial() = default;
  svtkOBJImportedPolyDataWithMaterial()
  { // initialize some structures to store the file contents in
    points = svtkSmartPointer<svtkPoints>::New();
    tcoords = svtkSmartPointer<svtkFloatArray>::New();
    normals = svtkSmartPointer<svtkFloatArray>::New();
    polys = svtkSmartPointer<svtkCellArray>::New();
    tcoord_polys = svtkSmartPointer<svtkCellArray>::New();
    pointElems = svtkSmartPointer<svtkCellArray>::New();
    lineElems = svtkSmartPointer<svtkCellArray>::New();
    normal_polys = svtkSmartPointer<svtkCellArray>::New();
    tcoords->SetNumberOfComponents(2);
    normals->SetNumberOfComponents(3);

    materialName = "";
    mtlProperties = nullptr;
  }

  // these can be shared
  svtkSmartPointer<svtkPoints> points;
  svtkSmartPointer<svtkFloatArray> normals;

  void SetSharedPoints(svtkSmartPointer<svtkPoints> arg) { points = arg; }
  void SetSharedNormals(svtkSmartPointer<svtkFloatArray> arg) { normals = arg; }

  // these are unique per entity
  svtkSmartPointer<svtkFloatArray> tcoords;
  svtkSmartPointer<svtkCellArray> polys;
  svtkSmartPointer<svtkCellArray> tcoord_polys;
  svtkSmartPointer<svtkCellArray> pointElems;
  svtkSmartPointer<svtkCellArray> lineElems;
  svtkSmartPointer<svtkCellArray> normal_polys;

  typedef std::map<std::string, svtkOBJImportedPolyDataWithMaterial*> NamedMaterials;
  std::string materialName;
  svtkOBJImportedMaterial* mtlProperties;
};

//----------------------------------------------------------------------------
svtkOBJPolyDataProcessor::svtkOBJPolyDataProcessor()
{
  // Instantiate object with nullptr filename, and no materials yet loaded.
  this->FileName = "";
  this->MTLFileName = "";
  this->DefaultMTLFileName = true;
  this->TexturePath = ".";
  this->VertexScale = 1.0;
  this->SuccessParsingFiles = 1;
  this->SetNumberOfInputPorts(0);
  /** multi-poly-data paradigm: pivot based on named materials */
  svtkOBJImportedPolyDataWithMaterial* default_poly = (new svtkOBJImportedPolyDataWithMaterial);
  poly_list.push_back(default_poly);
  this->SetNumberOfOutputPorts(static_cast<int>(poly_list.size()));
}

//----------------------------------------------------------------------------
svtkOBJPolyDataProcessor::~svtkOBJPolyDataProcessor()
{
  // clear any old mtls
  for (size_t k = 0; k < this->parsedMTLs.size(); ++k)
  {
    delete this->parsedMTLs[k];
  }

  for (size_t k = 0; k < poly_list.size(); ++k)
  {
    delete poly_list[k];
    poly_list[k] = nullptr;
  }
}
//----------------------------------------------------------------------------
int svtkOBJPolyDataProcessor::GetNumberOfOutputs()
{
  return static_cast<int>(poly_list.size());
}

//----------------------------------------------------------------------------
svtkOBJImportedMaterial* svtkOBJPolyDataProcessor::GetMaterial(int k)
{
  if (k >= static_cast<int>(poly_list.size()))
  {
    return nullptr;
  }
  svtkOBJImportedPolyDataWithMaterial* rpdmm = this->poly_list[k];
  return rpdmm->mtlProperties;
}

//----------------------------------------------------------------------------
std::string svtkOBJPolyDataProcessor::GetTextureFilename(int idx)
{
  svtkOBJImportedMaterial* mtl = this->GetMaterial(idx);

  if (mtl && !mtl->texture_filename.empty())
  {
    std::vector<std::string> path_and_filename(2);
    path_and_filename[0] = this->TexturePath;
    path_and_filename[1] = mtl->texture_filename;
    std::string joined = svtksys::SystemTools::JoinPath(path_and_filename);
    return joined;
  }

  return std::string();
}

// initialize some structures to store the file contents in

/*---------------------------------------------------------------------------*\

This is only partial support for the OBJ format, which is quite complicated.
To find a full specification, search the net for "OBJ format", eg.:

    http://en.wikipedia.org/wiki/Obj
    http://netghost.narod.ru/gff/graphics/summary/waveobj.htm

We support the following types:

v <x> <y> <z>

    vertex

vn <x> <y> <z>

    vertex normal

vt <x> <y>

    texture coordinate

f <v_a> <v_b> <v_c> ...

    polygonal face linking vertices v_a, v_b, v_c, etc. which
    are 1-based indices into the vertex list

f <v_a>/<t_a> <v_b>/<t_b> ...

    polygonal face as above, but with texture coordinates for
    each vertex. t_a etc. are 1-based indices into the texture
    coordinates list (from the vt lines)

f <v_a>/<t_a>/<n_a> <v_b>/<t_b>/<n_b> ...

    polygonal face as above, with a normal at each vertex, as a
    1-based index into the normals list (from the vn lines)

f <v_a>//<n_a> <v_b>//<n_b> ...

    polygonal face as above but without texture coordinates.

    Per-face tcoords and normals are supported by duplicating
    the vertices on each face as necessary.

l <v_a> <v_b> ...

    lines linking vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

p <v_a> <v_b> ...

    points located at the vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

\*---------------------------------------------------------------------------*/

//----------------------------------------------------------------------------
int svtkOBJPolyDataProcessor::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  if (this->FileName.empty())
  {
    svtkErrorMacro(<< "A FileName must be specified.");
    return 0;
  }

  FILE* in = svtksys::SystemTools::Fopen(this->FileName, "r");
  if (in == nullptr)
  {
    svtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  // clear old poly list
  for (size_t k = 0; k < poly_list.size(); ++k)
  {
    delete poly_list[k];
    poly_list[k] = nullptr;
  }
  poly_list.clear();

  svtkDebugMacro(<< "Reading file" << this->FileName);

  // clear any old mtls
  for (size_t k = 0; k < this->parsedMTLs.size(); ++k)
  {
    delete this->parsedMTLs[k];
  }

  // If the MTLFileName is not set explicitly, we assume *.obj.mtl as the MTL
  // filename
  std::string mtlname = this->MTLFileName;
  if (this->DefaultMTLFileName)
  {
    mtlname = this->FileName + ".mtl";
  }
  FILE* defMTL = svtksys::SystemTools::Fopen(mtlname, "r");
  if (defMTL == nullptr)
  {
    if (!this->DefaultMTLFileName)
    {
      svtkErrorMacro(<< "The MTL file " << mtlname << " could not be found");
      fclose(in);
      return 0;
    }
  }
  else
  {
    this->MTLFileName = mtlname;
    fclose(defMTL);
  }

  int mtlParseResult;
  this->parsedMTLs = ParseOBJandMTL(MTLFileName, mtlParseResult);
  if (this->parsedMTLs.empty())
  { // construct a default material to define the single polydata's actor.
    this->parsedMTLs.push_back(new svtkOBJImportedMaterial);
  }

  svtkDebugMacro("svtkOBJPolyDataProcessor parsed " << this->parsedMTLs.size() << " materials from "
                                                  << MTLFileName);

  svtkSmartPointer<svtkPoints> shared_vertexs = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkFloatArray> shared_normals = svtkSmartPointer<svtkFloatArray>::New();
  shared_normals->SetNumberOfComponents(3);

  std::map<std::string, std::vector<svtkOBJImportedPolyDataWithMaterial*> > mtlName_to_Actors;

  {
    // always have at least one output
    svtkOBJImportedPolyDataWithMaterial* newMaterial = new svtkOBJImportedPolyDataWithMaterial;
    newMaterial->SetSharedPoints(shared_vertexs);
    newMaterial->SetSharedNormals(shared_normals);
    poly_list.push_back(newMaterial);
    poly_list[0]->mtlProperties = parsedMTLs[0];

    mtlName_to_mtlData.clear();
    for (size_t k = 0; k < this->parsedMTLs.size(); ++k)
    {
      std::string mtlname_k(this->parsedMTLs[k]->name);
      mtlName_to_mtlData[mtlname_k] = this->parsedMTLs[k];
    }
  }

  svtkPoints* points = poly_list.back()->points;
  svtkFloatArray* tcoords = poly_list.back()->tcoords;
  svtkFloatArray* normals = poly_list.back()->normals;
  svtkCellArray* polys = poly_list.back()->polys;
  svtkCellArray* tcoord_polys = poly_list.back()->tcoord_polys;
  svtkCellArray* pointElems = poly_list.back()->pointElems;
  svtkCellArray* lineElems = poly_list.back()->lineElems;
  svtkCellArray* normal_polys = poly_list.back()->normal_polys;

  bool gotFirstUseMaterialTag = false;

  int numPolysWithTCoords = 0;
  bool hasTCoords = false;                 // has vt x y z
  bool hasPolysWithTextureIndices = false; // has f i/t/n or f i/t
  bool hasNormals = false;                 // has f i/t/n or f i//n
  bool tcoords_same_as_verts = true;
  bool normals_same_as_verts = true;
  bool everything_ok = true; // (use of this flag avoids early return and associated memory leak)
  const double v_scale = this->VertexScale;
  const bool use_scale = (fabs(v_scale - 1.0) > 1e-3);

  // -- work through the file line by line, assigning into the above 7 structures as appropriate --
  { // (make a local scope section to emphasise that the variables below are only used here)

    const int MAX_LINE = 100000;
    char rawLine[MAX_LINE];
    float xyz[3];

    int lineNr = 0;
    while (everything_ok && fgets(rawLine, MAX_LINE, in) != nullptr)
    { /** While OK and there is another line in the file */
      lineNr++;
      char* pLine = rawLine;
      char* pEnd = rawLine + strlen(rawLine);

      // watch for BOM
      if (pEnd - pLine > 3 && pLine[0] == -17 && pLine[1] == -69 && pLine[2] == -65)
      {
        pLine += 3;
      }

      // find the first non-whitespace character
      while (isspace(*pLine) && pLine < pEnd)
      {
        pLine++;
      }

      // this first non-whitespace is the command
      const char* cmd = pLine;

      // skip over non-whitespace
      while (!isspace(*pLine) && pLine < pEnd)
      {
        pLine++;
      }

      // terminate command
      if (pLine < pEnd)
      {
        *pLine = '\0';
        pLine++;
      }

      // in the OBJ format the first characters determine how to interpret the line:
      static long lastVertexIndex = 0;
      if (strcmp(cmd, "v") == 0)
      {
        // this is a vertex definition, expect three floats, separated by whitespace:
        if (sscanf(pLine, "%f %f %f", xyz, xyz + 1, xyz + 2) == 3)
        {
          if (use_scale)
          {
            xyz[0] *= v_scale;
            xyz[1] *= v_scale;
            xyz[2] *= v_scale;
          }
          points->InsertNextPoint(xyz);
          lastVertexIndex++;
        }
        else
        {
          svtkErrorMacro(<< "Error reading 'v' at line " << lineNr);
          everything_ok = false;
        }
        if (gotFirstUseMaterialTag && this->GetDebug())
        {
          svtkWarningMacro("attempting to add vertices after usemtl ... ");
        }
      }
      else if (strcmp(cmd, "vt") == 0) /** Texture Coord, whango! */
      {
        // this is a tcoord, expect two floats, separated by whitespace:
        if (sscanf(pLine, "%f %f", xyz, xyz + 1) == 2)
        {
          tcoords->InsertNextTuple(xyz);
        }
        else
        {
          svtkErrorMacro(<< "Error reading 'vt' at line " << lineNr);
          everything_ok = false;
        }
      }
      else if (strcmp(cmd, "vn") == 0)
      {
        // this is a normal, expect three floats, separated by whitespace:
        if (sscanf(pLine, "%f %f %f", xyz, xyz + 1, xyz + 2) == 3)
        {
          normals->InsertNextTuple(xyz);
          hasNormals = true;
        }
        else
        {
          svtkErrorMacro(<< "Error reading 'vn' at line " << lineNr);
          everything_ok = false;
        }
      }
      else if (strcmp(cmd, "p") == 0)
      {
        // this is a point definition, consisting of 1-based indices separated by whitespace and /
        pointElems->InsertNextCell(0); // we don't yet know how many points are to come

        int nVerts = 0; // keep a count of how many there are

        while (everything_ok && pLine < pEnd)
        {
          // find next non-whitespace character
          while (isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }

          if (pLine < pEnd) // there is still data left on this line
          {
            int iVert;
            if (sscanf(pLine, "%d", &iVert) == 1)
            {
              pointElems->InsertCellPoint(iVert - 1);
              nVerts++;
            }
            else if (strcmp(pLine, "\\\n") == 0)
            {
              // handle backslash-newline continuation
              if (fgets(rawLine, MAX_LINE, in) != nullptr)
              {
                lineNr++;
                pLine = rawLine;
                pEnd = rawLine + strlen(rawLine);
                continue;
              }
              else
              {
                svtkErrorMacro(<< "Error reading continuation line at line " << lineNr);
                everything_ok = false;
              }
            }
            else
            {
              svtkErrorMacro(<< "Error reading 'p' at line " << lineNr);
              everything_ok = false;
            }
            // skip over what we just sscanf'd
            // (find the first whitespace character)
            while (!isspace(*pLine) && pLine < pEnd)
            {
              pLine++;
            }
          }
        }

        if (nVerts < 1)
        {
          svtkErrorMacro(<< "Error reading file near line " << lineNr
                        << " while processing the 'p' command");
          everything_ok = false;
        }

        // now we know how many points there were in this cell
        pointElems->UpdateCellCount(nVerts);
      }
      else if (strcmp(cmd, "l") == 0)
      {
        // this is a line definition, consisting of 1-based indices separated by whitespace and /
        lineElems->InsertNextCell(0); // we don't yet know how many points are to come

        int nVerts = 0; // keep a count of how many there are

        while (everything_ok && pLine < pEnd)
        {
          // find next non-whitespace character
          while (isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }

          if (pLine < pEnd) // there is still data left on this line
          {
            int iVert, dummyInt;
            if (sscanf(pLine, "%d/%d", &iVert, &dummyInt) == 2)
            {
              // we simply ignore texture information
              lineElems->InsertCellPoint(iVert - 1);
              nVerts++;
            }
            else if (sscanf(pLine, "%d", &iVert) == 1)
            {
              lineElems->InsertCellPoint(iVert - 1);
              nVerts++;
            }
            else if (strcmp(pLine, "\\\n") == 0)
            {
              // handle backslash-newline continuation
              if (fgets(rawLine, MAX_LINE, in) != nullptr)
              {
                lineNr++;
                pLine = rawLine;
                pEnd = rawLine + strlen(rawLine);
                continue;
              }
              else
              {
                svtkErrorMacro(<< "Error reading continuation line at line " << lineNr);
                everything_ok = false;
              }
            }
            else
            {
              svtkErrorMacro(<< "Error reading 'l' at line " << lineNr);
              everything_ok = false;
            }
            // skip over what we just sscanf'd
            // (find the first whitespace character)
            while (!isspace(*pLine) && pLine < pEnd)
            {
              pLine++;
            }
          }
        }

        if (nVerts < 2)
        {
          svtkErrorMacro(<< "Error reading file near line " << lineNr
                        << " while processing the 'l' command");
          everything_ok = false;
        }

        // now we know how many points there were in this cell
        lineElems->UpdateCellCount(nVerts);
      }
      else if (strcmp(cmd, "f") == 0)
      {
        // this is a face definition, consisting of 1-based indices separated by whitespace and /

        polys->InsertNextCell(0); // we don't yet know how many points are to come
        tcoord_polys->InsertNextCell(0);
        normal_polys->InsertNextCell(0);

        int nVerts = 0, nTCoords = 0, nNormals = 0; // keep a count of how many of each there are

        while (everything_ok && pLine < pEnd)
        {
          // find the first non-whitespace character
          while (isspace(*pLine) && pLine < pEnd)
          {
            pLine++;
          }

          if (pLine < pEnd) // there is still data left on this line
          {
            int iVert, iTCoord, iNormal;
            if (sscanf(pLine, "%d/%d/%d", &iVert, &iTCoord, &iNormal) == 3)
            {
              // negative indices are specified relative to the current maximum vertex
              // position.  (-1 references the last vertex defined). This makes it easy
              // to describe the points in a face, then the face, without the need to
              // store a large list of points and their indexes.
              if (iVert < 0)
              {
                iVert = lastVertexIndex + iVert + 1;
              }
              if (iTCoord < 0)
              {
                iTCoord = lastVertexIndex + iTCoord + 1;
              }
              if (iNormal < 0)
              {
                iNormal = lastVertexIndex + iNormal + 1;
              }
              hasPolysWithTextureIndices = true;
              polys->InsertCellPoint(iVert - 1); // convert to 0-based index
              nVerts++;
              tcoord_polys->InsertCellPoint(iTCoord - 1);
              nTCoords++;
              normal_polys->InsertCellPoint(iNormal - 1);
              nNormals++;
              if (iTCoord != iVert)
                tcoords_same_as_verts = false;
              if (iNormal != iVert)
                normals_same_as_verts = false;
            }
            else if (sscanf(pLine, "%d//%d", &iVert, &iNormal) == 2)
            {
              if (iVert < 0)
              {
                iVert = lastVertexIndex + iVert + 1;
              }
              if (iNormal < 0)
              {
                iNormal = lastVertexIndex + iNormal + 1;
              }
              hasPolysWithTextureIndices = false;
              polys->InsertCellPoint(iVert - 1);
              nVerts++;
              normal_polys->InsertCellPoint(iNormal - 1);
              nNormals++;
              if (iNormal != iVert)
                normals_same_as_verts = false;
            }
            else if (sscanf(pLine, "%d/%d", &iVert, &iTCoord) == 2)
            {
              if (iVert < 0)
              {
                iVert = lastVertexIndex + iVert + 1;
              }
              if (iTCoord < 0)
              {
                iTCoord = lastVertexIndex + iTCoord + 1;
              }
              hasPolysWithTextureIndices = true;
              polys->InsertCellPoint(iVert - 1);
              nVerts++;
              tcoord_polys->InsertCellPoint(iTCoord - 1);
              nTCoords++;
              if (iTCoord != iVert)
                tcoords_same_as_verts = false;
            }
            else if (sscanf(pLine, "%d", &iVert) == 1)
            {
              if (iVert < 0)
              {
                iVert = lastVertexIndex + iVert + 1;
              }
              hasPolysWithTextureIndices = false;
              polys->InsertCellPoint(iVert - 1);
              nVerts++;
            }
            else if (strcmp(pLine, "\\\n") == 0)
            {
              // handle backslash-newline continuation
              if (fgets(rawLine, MAX_LINE, in) != nullptr)
              {
                lineNr++;
                pLine = rawLine;
                pEnd = rawLine + strlen(rawLine);
                continue;
              }
              else
              {
                svtkErrorMacro(<< "Error reading continuation line at line " << lineNr);
                everything_ok = false;
              }
            }
            else
            {
              svtkErrorMacro(<< "Error reading 'f' at line " << lineNr);
              everything_ok = false;
            }
            // skip over what we just read
            // (find the first whitespace character)
            while (!isspace(*pLine) && pLine < pEnd)
            {
              pLine++;
            }
          }
        }

        // count of tcoords and normals must be equal to number of vertices or zero
        if (nVerts < 3 || (nTCoords > 0 && nTCoords != nVerts) ||
          (nNormals > 0 && nNormals != nVerts))
        {
          svtkErrorMacro(<< "Error reading file near line " << lineNr
                        << " while processing the 'f' command"
                        << " nVerts= " << nVerts << " nTCoords= " << nTCoords
                        << " nNormals= " << nNormals << pLine);
          everything_ok = false;
        }

        // now we know how many points there were in this cell
        polys->UpdateCellCount(nVerts);
        tcoord_polys->UpdateCellCount(nTCoords);
        normal_polys->UpdateCellCount(nNormals);

        // also make a note of whether any cells have tcoords, and whether any have normals
        numPolysWithTCoords += (int)(nTCoords) > 0;
        if ((!hasTCoords) && (nTCoords > 0))
        {
          svtkDebugMacro("got texture coords in obj file! nTCoords = " << nTCoords);
          hasTCoords = true;
        }
        else if (nTCoords == 0)
        {
          svtkDebugMacro("did NOT get texture coords in obj file!");
        }
        if (nNormals > 0)
        {
          hasNormals = true;
        }
      }
      else if (strcmp(cmd, "usemtl") == 0)
      {
        // find the first non-whitespace character
        while (isspace(*pLine) && pLine < pEnd)
        {
          pLine++;
        }
        std::string strLine(pLine);
        svtkDebugMacro("strLine = " << strLine);
        size_t idxNewLine = strLine.find_first_of("\r\n");
        std::string mtl_name = strLine.substr(0, idxNewLine);
        // trim trailing whitespace
        size_t last = mtl_name.find_last_not_of(' ');
        mtl_name = mtl_name.substr(0, last + 1);
        svtkDebugMacro("'Use Material' command, usemtl with name: " << mtl_name);

        if (!mtlName_to_mtlData.count(mtl_name))
        {
          svtkErrorMacro(" material '" << mtl_name << "' appears in OBJ but not MTL file?");
        }
        // if this is the first usemtl then assign it to the
        // poly_list[0]
        if (!gotFirstUseMaterialTag)
        {
          poly_list[0]->materialName = mtl_name;
          poly_list[0]->mtlProperties = mtlName_to_mtlData[mtl_name];
          mtlName_to_Actors[mtl_name].push_back(poly_list[0]);
          // yep we have a usemtl command. check to make sure idiots don't try to add vertices
          // later.
          gotFirstUseMaterialTag = true;
        }
        // create a new materia
        svtkOBJImportedPolyDataWithMaterial* newMaterial = new svtkOBJImportedPolyDataWithMaterial;
        newMaterial->SetSharedPoints(shared_vertexs);
        newMaterial->SetSharedNormals(shared_normals);
        poly_list.push_back(newMaterial);

        poly_list.back()->materialName = mtl_name;
        poly_list.back()->mtlProperties = mtlName_to_mtlData[mtl_name];
        mtlName_to_Actors[mtl_name].push_back(poly_list.back());

        svtkOBJImportedPolyDataWithMaterial* active = newMaterial;
        polys = active->polys; // Update pointers reading file further
        tcoord_polys = active->tcoord_polys;
        pointElems = active->pointElems;
        lineElems = active->lineElems;
        normal_polys = active->normal_polys;
      }
      else
      {
        svtkDebugMacro(<< "Ignoring line: " << rawLine);
      }
    } /** Looping over lines of file */ // (end of while loop)
  }                                     // (end of local scope section)

  // we have finished with the file
  fclose(in);

  /** based on how many used materials are present,
                 set the number of output ports of svtkPolyData */
  this->SetNumberOfOutputPorts(static_cast<int>(poly_list.size()));
  svtkDebugMacro("svtkOBJPolyDataProcessor.cxx, set # of output ports to " << poly_list.size());
  this->outVector_of_svtkPolyData.clear();
  for (size_t i = 0; i < poly_list.size(); ++i)
  {
    svtkSmartPointer<svtkPolyData> poly_data = svtkSmartPointer<svtkPolyData>::New();
    this->outVector_of_svtkPolyData.push_back(poly_data);
  }

  if (everything_ok) // (otherwise just release allocated memory and return)
  {
    // -- now turn this lot into a useable svtkPolyData --
    // loop over the materials found in the obj file
    for (size_t outputIndex = 0; outputIndex < poly_list.size(); ++outputIndex)
    {
      svtkOBJImportedPolyDataWithMaterial* active = poly_list[outputIndex];
      svtkSmartPointer<svtkPolyData> output = outVector_of_svtkPolyData[outputIndex];
      polys = active->polys; // Update pointers reading file further
      tcoord_polys = active->tcoord_polys;
      pointElems = active->pointElems;
      lineElems = active->lineElems;
      normal_polys = active->normal_polys;
      svtkDebugMacro("generating output polydata ....  \n"
        << "tcoords same as verts!? " << tcoords_same_as_verts << " ... hasTCoords?" << hasTCoords
        << " ... numPolysWithTCoords = " << numPolysWithTCoords);
      // if there are no tcoords or normals or they match exactly
      // then we can just copy the data into the output (easy!)
      if ((!hasTCoords || tcoords_same_as_verts) && (!hasNormals || normals_same_as_verts))
      { // ...
        svtkDebugMacro(<< "Copying file data into the output directly");

        output->SetPoints(points);
        if (pointElems->GetNumberOfCells())
        {
          output->SetVerts(pointElems);
        }
        if (lineElems->GetNumberOfCells())
        {
          output->SetLines(lineElems);
        }
        if (polys->GetNumberOfCells())
        {
          output->SetPolys(polys);
        }

        // if there is an exact correspondence between tcoords and vertices then can simply
        // assign the tcoords points as point data
        if (hasTCoords && tcoords_same_as_verts && hasPolysWithTextureIndices)
          output->GetPointData()->SetTCoords(tcoords);

        // if there is an exact correspondence between normals and vertices then can simply
        // assign the normals as point data
        if (hasNormals && normals_same_as_verts)
        {
          output->GetPointData()->SetNormals(normals);
        }
        output->Squeeze();
      }
      // otherwise we can duplicate the vertices as necessary (a bit slower)
      else
      {
        svtkDebugMacro(<< "Duplicating vertices so that tcoords and normals are correct");
        svtkPoints* new_points = svtkPoints::New();
        svtkFloatArray* new_tcoords = svtkFloatArray::New();
        new_tcoords->SetNumberOfComponents(2);
        svtkFloatArray* new_normals = svtkFloatArray::New();
        new_normals->SetNumberOfComponents(3);
        svtkCellArray* new_polys = svtkCellArray::New();

        // for each poly, copy its vertices into new_points (and point at them)
        // also copy its tcoords into new_tcoords
        // also copy its normals into new_normals
        polys->InitTraversal();
        tcoord_polys->InitTraversal();
        normal_polys->InitTraversal();

        svtkIdType n_pts = -1;
        const svtkIdType* pts;
        svtkIdType n_tcoord_pts = -1;
        const svtkIdType* tcoord_pts;
        svtkIdType n_normal_pts = -1;
        const svtkIdType* normal_pts;

        svtkNew<svtkIdList> tmpCell;

        for (int i = 0; i < polys->GetNumberOfCells(); ++i)
        {
          polys->GetNextCell(n_pts, pts);
          tcoord_polys->GetNextCell(n_tcoord_pts, tcoord_pts);
          normal_polys->GetNextCell(n_normal_pts, normal_pts);

          // If some vertices have tcoords and not others (likewise normals)
          // then we must do something else SVTK will complain. (crash on render attempt)
          // Easiest solution is to delete polys that don't have complete tcoords (if there
          // are any tcoords in the dataset) or normals (if there are any normals in the dataset).

          if ((n_pts != n_tcoord_pts && hasTCoords && hasPolysWithTextureIndices) ||
            (n_pts != n_normal_pts && hasNormals))
          {
            // skip this poly
            svtkDebugMacro(<< "Skipping poly " << i + 1 << " (1-based index)");
          }
          else
          {
            tmpCell->SetNumberOfIds(n_pts);
            // copy the corresponding points, tcoords and normals across
            for (int j = 0; j < n_pts; ++j)
            {
              // copy the tcoord for this point across (if there is one)
              if (n_tcoord_pts > 0 && hasPolysWithTextureIndices)
              {
                new_tcoords->InsertNextTuple(tcoords->GetTuple(tcoord_pts[j]));
              }
              // copy the normal for this point across (if there is one)
              if (n_normal_pts > 0)
              {
                new_normals->InsertNextTuple(normals->GetTuple(normal_pts[j]));
              }
              // copy the vertex into the new structure and update
              // the vertex index in the polys structure (pts is a pointer into it)
              tmpCell->SetId(j, new_points->InsertNextPoint(points->GetPoint(pts[j])));
            }
            polys->ReplaceCellAtId(i, tmpCell);
            // copy this poly (pointing at the new points) into the new polys list
            new_polys->InsertNextCell(tmpCell);
          }
        }

        // use the new structures for the output
        output->SetPoints(new_points);
        output->SetPolys(new_polys);
        svtkDebugMacro(" set new points, count = " << new_points->GetNumberOfPoints() << " ...");
        svtkDebugMacro(" set new polys, count = " << new_polys->GetNumberOfCells() << " ...");

        if (hasTCoords && hasPolysWithTextureIndices)
        {
          output->GetPointData()->SetTCoords(new_tcoords);
          svtkDebugMacro(" set new tcoords");
        }
        if (hasNormals)
        {
          output->GetPointData()->SetNormals(new_normals);
          svtkDebugMacro(" set new normals");
        }

        // TODO: fixup for pointElems and lineElems too
        output->Squeeze();

        new_points->Delete();
        new_polys->Delete();
        new_tcoords->Delete();
        new_normals->Delete();
      }
    }
  }

  if (!everything_ok)
  {
    SetSuccessParsingFiles(false);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkOBJPolyDataProcessor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName.empty() ? this->FileName : "(none)") << "\n";
  os << indent << "MTLFileName: " << (this->MTLFileName.empty() ? this->MTLFileName : "(none)")
     << "\n";
  os << indent << "TexturePath: " << (this->TexturePath.empty() ? this->TexturePath : "(none)")
     << "\n";
}

//----------------------------------------------------------------------------
svtkPolyData* svtkOBJPolyDataProcessor::GetOutput(int idx)
{
  if (idx < (int)outVector_of_svtkPolyData.size())
  {
    return outVector_of_svtkPolyData[idx];
  }
  else
  {
    return nullptr;
  }
}
