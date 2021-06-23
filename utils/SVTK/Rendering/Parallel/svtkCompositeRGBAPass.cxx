/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeRGBAPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCompositeRGBAPass.h"
#include "svtkFrameBufferObjectBase.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkRenderState.h"
#include "svtkTextureObject.h"
#include <cassert>

// to be able to dump intermediate result into png files for debugging.
// only for svtkCompositeRGBAPass developers.
//#define SVTK_COMPOSITE_RGBAPASS_DEBUG

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageExtractComponents.h"
#include "svtkImageImport.h"
#include "svtkImageShiftScale.h"
#include "svtkIntArray.h"
#include "svtkMultiProcessController.h"
#include "svtkPKdTree.h"
#include "svtkPNGWriter.h"
#include "svtkPixelBufferObject.h"
#include "svtkPointData.h"
#include "svtkStdString.h"
#include "svtkTimerLog.h"
#include <sstream>

#ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG
//#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h> // Linux specific gettid()
#endif

#include "svtk_glew.h"

svtkStandardNewMacro(svtkCompositeRGBAPass);
svtkCxxSetObjectMacro(svtkCompositeRGBAPass, Controller, svtkMultiProcessController);
svtkCxxSetObjectMacro(svtkCompositeRGBAPass, Kdtree, svtkPKdTree);

// ----------------------------------------------------------------------------
svtkCompositeRGBAPass::svtkCompositeRGBAPass()
{
  this->Controller = nullptr;
  this->Kdtree = nullptr;
  this->PBO = nullptr;
  this->RGBATexture = nullptr;
  this->RootTexture = nullptr;
  this->RawRGBABuffer = nullptr;
  this->RawRGBABufferSize = 0;
}

// ----------------------------------------------------------------------------
svtkCompositeRGBAPass::~svtkCompositeRGBAPass()
{
  if (this->Controller != nullptr)
  {
    this->Controller->Delete();
  }
  if (this->Kdtree != nullptr)
  {
    this->Kdtree->Delete();
  }
  if (this->PBO != nullptr)
  {
    svtkErrorMacro(<< "PixelBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->RGBATexture != nullptr)
  {
    svtkErrorMacro(<< "RGBATexture should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->RootTexture != nullptr)
  {
    svtkErrorMacro(<< "RootTexture should have been deleted in ReleaseGraphicsResources().");
  }
  delete[] this->RawRGBABuffer;
}

// ----------------------------------------------------------------------------
void svtkCompositeRGBAPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller:";
  if (this->Controller != nullptr)
  {
    this->Controller->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "Kdtree:";
  if (this->Kdtree != nullptr)
  {
    this->Kdtree->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
bool svtkCompositeRGBAPass::IsSupported(svtkOpenGLRenderWindow* context)
{
  return (context != nullptr);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkCompositeRGBAPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  if (this->Controller == nullptr)
  {
    svtkErrorMacro(<< " no controller.");
    return;
  }

  int numProcs = this->Controller->GetNumberOfProcesses();

  if (numProcs == 1)
  {
    return; // nothing to do.
  }

  if (this->Kdtree == nullptr)
  {
    svtkErrorMacro(<< " no Kdtree.");
    return;
  }

  int me = this->Controller->GetLocalProcessId();

  const int SVTK_COMPOSITE_RGBA_PASS_MESSAGE_GATHER = 201;

  svtkOpenGLRenderer* r = static_cast<svtkOpenGLRenderer*>(s->GetRenderer());

  svtkOpenGLRenderWindow* context = static_cast<svtkOpenGLRenderWindow*>(r->GetRenderWindow());
  svtkOpenGLState* ostate = context->GetState();

  if (!this->IsSupported(context))
  {
    svtkErrorMacro(<< "Missing required OpenGL extensions. "
                  << "Cannot perform rgba-compositing.");
    return;
  }

  int w = 0;
  int h = 0;

  svtkFrameBufferObjectBase* fbo = s->GetFrameBuffer();
  if (fbo == nullptr)
  {
    r->GetTiledSize(&w, &h);
  }
  else
  {
    int size[2];
    fbo->GetLastSize(size);
    w = size[0];
    h = size[1];
  }

  int numComps = 4;
  unsigned int numTups = w * h;

  // pbo arguments.
  unsigned int dims[2];
  svtkIdType continuousInc[3];

  dims[0] = static_cast<unsigned int>(w);
  dims[1] = static_cast<unsigned int>(h);
  continuousInc[0] = 0;
  continuousInc[1] = 0;
  continuousInc[2] = 0;

  if (this->RawRGBABuffer != nullptr && this->RawRGBABufferSize < static_cast<size_t>(w * h * 4))
  {
    delete[] this->RawRGBABuffer;
    this->RawRGBABuffer = nullptr;
  }
  if (this->RawRGBABuffer == nullptr)
  {
    this->RawRGBABufferSize = static_cast<size_t>(w * h * 4);
    this->RawRGBABuffer = new float[this->RawRGBABufferSize];
  }

  // size_t byteSize = this->RawRGBABufferSize*sizeof(unsigned char);

  if (this->PBO == nullptr)
  {
    this->PBO = svtkPixelBufferObject::New();
    this->PBO->SetContext(context);
  }
  if (this->RGBATexture == nullptr)
  {
    this->RGBATexture = svtkTextureObject::New();
    this->RGBATexture->SetContext(context);
  }

  // TO: texture object
  // PBO: pixel buffer object
  // FB: framebuffer

#ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG
  svtkTimerLog* timer = svtkTimerLog::New();

  svtkImageImport* importer;
  svtkImageShiftScale* converter;
  svtkPNGWriter* writer;

  cout << "me=" << me << " TID=" << syscall(SYS_gettid) << " thread=" << pthread_self() << endl;
  timer->StartTimer();
#endif

  if (me == 0)
  {
    // root
    // 1. figure out the back to front ordering
    // 2. if root is not farest, save it in a TO
    // 3. in back to front order:
    // 3a. if this is step for root, render root TO (if not farest)
    // 3b. if satellite, get image, load it into TO, render quad

#ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG
    // get rgba-buffer of root before any blending with satellite
    // for debugging only.

    // Framebuffer to PBO
    this->PBO->Allocate(SVTK_FLOAT, numTups, numComps, svtkPixelBufferObject::PACKED_BUFFER);

    this->PBO->Bind(svtkPixelBufferObject::PACKED_BUFFER);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, static_cast<GLfloat*>(nullptr));
    cout << "after readpixel." << endl;
    this->PBO->Download2D(SVTK_FLOAT, this->RawRGBABuffer, dims, 4, continuousInc);
    cout << "after pbodownload." << endl;
    importer = svtkImageImport::New();
    importer->CopyImportVoidPointer(this->RawRGBABuffer, static_cast<int>(byteSize));
    importer->SetDataScalarTypeToFloat();
    importer->SetNumberOfScalarComponents(4);
    importer->SetWholeExtent(0, w - 1, 0, h - 1, 0, 0);
    importer->SetDataExtentToWholeExtent();

    importer->Update();
    cout << "after importer update" << endl;
    converter = svtkImageShiftScale::New();
    converter->SetInputConnection(importer->GetOutputPort());
    converter->SetOutputScalarTypeToUnsignedChar();
    converter->SetShift(0.0);
    converter->SetScale(255.0);

    //      svtkImageExtractComponents *rgbaToRgb=svtkImageExtractComponents::New();
    //    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
    //    rgbaToRgb->SetComponents(0,1,2);

    std::ostringstream ostxx;
    ostxx.setf(ios::fixed, ios::floatfield);
    ostxx.precision(5);
    timer->StopTimer();
    ostxx << "root0_" << svtkTimerLog::GetUniversalTime() << "_.png";

    svtkStdString* sssxx = new svtkStdString;
    (*sssxx) = ostxx.str();

    writer = svtkPNGWriter::New();
    writer->SetFileName(*sssxx);
    delete sssxx;
    writer->SetInputConnection(converter->GetOutputPort());
    converter->Delete();
    importer->Delete();
    //    rgbaToRgb->Delete();
    cout << "Writing " << writer->GetFileName() << endl;
    writer->Write();
    cout << "Wrote " << writer->GetFileName() << endl;
    //    sleep(30);
    writer->Delete();
#endif // #ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG

    // 1. figure out the back to front ordering
    svtkCamera* c = r->GetActiveCamera();
    svtkIntArray* frontToBackList = svtkIntArray::New();
    if (c->GetParallelProjection())
    {
      this->Kdtree->ViewOrderAllProcessesInDirection(
        c->GetDirectionOfProjection(), frontToBackList);
    }
    else
    {
      this->Kdtree->ViewOrderAllProcessesFromPosition(c->GetPosition(), frontToBackList);
    }

    assert("check same_size" && frontToBackList->GetNumberOfTuples() == numProcs);

#ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG
    int i = 0;
    while (i < numProcs)
    {
      cout << "frontToBackList[" << i << "]=" << frontToBackList->GetValue(i) << endl;
      ++i;
    }
#endif

    // framebuffers have their color premultiplied by alpha.

    // save off current state of src / dst blend functions
    {
      svtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(ostate);

      ostate->svtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

      // per-fragment operations
      ostate->svtkglDisable(GL_DEPTH_TEST);
      ostate->svtkglDisable(GL_BLEND);
      ostate->svtkglBlendFuncSeparate(
        GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // client to server

      // 2. if root is not farest, save it in a TO
      bool rootIsFarest = frontToBackList->GetValue(numProcs - 1) == 0;
      if (!rootIsFarest)
      {
        if (this->RootTexture == nullptr)
        {
          this->RootTexture = svtkTextureObject::New();
          this->RootTexture->SetContext(context);
        }
        this->RootTexture->Allocate2D(dims[0], dims[1], 4, SVTK_UNSIGNED_CHAR);
        this->RootTexture->CopyFromFrameBuffer(0, 0, 0, 0, w, h);
      }

      // 3. in back to front order:
      // 3a. if this is step for root, render root TO (if not farest)
      // 3b. if satellite, get image, load it into TO, render quad

      int procIndex = numProcs - 1;
      bool blendingEnabled = false;
      if (rootIsFarest)
      {
        // nothing to do.
        --procIndex;
      }
      while (procIndex >= 0)
      {
        svtkTextureObject* to;
        int proc = frontToBackList->GetValue(procIndex);
        if (proc == 0)
        {
          to = this->RootTexture;
        }
        else
        {
          // receive the rgba from satellite process.
          this->Controller->Receive(this->RawRGBABuffer,
            static_cast<svtkIdType>(this->RawRGBABufferSize), proc,
            SVTK_COMPOSITE_RGBA_PASS_MESSAGE_GATHER);

          // send it to a PBO
          this->PBO->Upload2D(SVTK_FLOAT, this->RawRGBABuffer, dims, 4, continuousInc);
          // Send PBO to TO
          this->RGBATexture->Create2D(dims[0], dims[1], 4, this->PBO, false);
          to = this->RGBATexture;
        }
        if (!blendingEnabled && procIndex < (numProcs - 1))
        {
          ostate->svtkglEnable(GL_BLEND);
          blendingEnabled = true;
        }
        to->Activate();
        to->CopyToFrameBuffer(0, 0, w - 1, h - 1, 0, 0, w, h, nullptr, nullptr);
        to->Deactivate();
        --procIndex;
      }
      // restore blend func by going out of scope
    }

    frontToBackList->Delete();

#ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG
    // get rgba-buffer of root before any blending with satellite
    // for debugging only.

    // Framebuffer to PBO
    this->PBO->Allocate(SVTK_FLOAT, numTups, numComps, svtkPixelBufferObject::PACKED_BUFFER);

    this->PBO->Bind(svtkPixelBufferObject::PACKED_BUFFER);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, static_cast<GLfloat*>(nullptr));

    this->PBO->Download2D(SVTK_FLOAT, this->RawRGBABuffer, dims, 4, continuousInc);

    importer = svtkImageImport::New();
    importer->CopyImportVoidPointer(this->RawRGBABuffer, static_cast<int>(byteSize));
    importer->SetDataScalarTypeToFloat();
    importer->SetNumberOfScalarComponents(4);
    importer->SetWholeExtent(0, w - 1, 0, h - 1, 0, 0);
    importer->SetDataExtentToWholeExtent();

    importer->Update();

    converter = svtkImageShiftScale::New();
    converter->SetInputConnection(importer->GetOutputPort());
    converter->SetOutputScalarTypeToUnsignedChar();
    converter->SetShift(0.0);
    converter->SetScale(255.0);

    //      svtkImageExtractComponents *rgbaToRgb=svtkImageExtractComponents::New();
    //    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
    //    rgbaToRgb->SetComponents(0,1,2);

    std::ostringstream osty;
    osty.setf(ios::fixed, ios::floatfield);
    osty.precision(5);
    timer->StopTimer();
    osty << "rootend_" << svtkTimerLog::GetUniversalTime() << "_.png";

    svtkStdString* sssy = new svtkStdString;
    (*sssy) = osty.str();

    writer = svtkPNGWriter::New();
    writer->SetFileName(*sssy);
    delete sssy;
    writer->SetInputConnection(converter->GetOutputPort());
    converter->Delete();
    importer->Delete();
    //    rgbaToRgb->Delete();
    cout << "Writing " << writer->GetFileName() << endl;
    writer->Write();
    cout << "Wrote " << writer->GetFileName() << endl;
    //    sleep(30);
    writer->Delete();
#endif

    // root node Done.
  }
  else
  {
    // satellite
    // send rgba-buffer

    // framebuffer to PBO.
    this->PBO->Allocate(SVTK_FLOAT, numTups, numComps, svtkPixelBufferObject::PACKED_BUFFER);

    this->PBO->Bind(svtkPixelBufferObject::PACKED_BUFFER);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, static_cast<GLfloat*>(nullptr));

    // PBO to client
    glPixelStorei(GL_PACK_ALIGNMENT, 1); // server to client
    this->PBO->Download2D(SVTK_FLOAT, this->RawRGBABuffer, dims, 4, continuousInc);
    this->PBO->UnBind();

#ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG
    importer = svtkImageImport::New();
    importer->CopyImportVoidPointer(this->RawRGBABuffer, static_cast<int>(byteSize));
    importer->SetDataScalarTypeToFloat();
    importer->SetNumberOfScalarComponents(4);
    importer->SetWholeExtent(0, w - 1, 0, h - 1, 0, 0);
    importer->SetDataExtentToWholeExtent();

    importer->Update();

    converter = svtkImageShiftScale::New();
    converter->SetInputConnection(importer->GetOutputPort());
    converter->SetOutputScalarTypeToUnsignedChar();
    converter->SetShift(0.0);
    converter->SetScale(255.0);

    //      svtkImageExtractComponents *rgbaToRgb=svtkImageExtractComponents::New();
    //    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
    //    rgbaToRgb->SetComponents(0,1,2);

    std::ostringstream ostxx;
    ostxx.setf(ios::fixed, ios::floatfield);
    ostxx.precision(5);
    timer->StopTimer();
    ostxx << "satellite_send_" << svtkTimerLog::GetUniversalTime() << "_.png";

    svtkStdString* sssxx = new svtkStdString;
    (*sssxx) = ostxx.str();

    writer = svtkPNGWriter::New();
    writer->SetFileName(*sssxx);
    delete sssxx;
    writer->SetInputConnection(converter->GetOutputPort());
    converter->Delete();
    importer->Delete();
    //    rgbaToRgb->Delete();
    cout << "Writing " << writer->GetFileName() << endl;
    writer->Write();
    cout << "Wrote " << writer->GetFileName() << endl;
    //    sleep(30);
    writer->Delete();
#endif

    // client to root process
    this->Controller->Send(this->RawRGBABuffer, static_cast<svtkIdType>(this->RawRGBABufferSize), 0,
      SVTK_COMPOSITE_RGBA_PASS_MESSAGE_GATHER);
  }
#ifdef SVTK_COMPOSITE_RGBAPASS_DEBUG
  delete state;
  timer->Delete();
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkCompositeRGBAPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  (void)w;

  if (this->PBO != nullptr)
  {
    this->PBO->Delete();
    this->PBO = nullptr;
  }
  if (this->RGBATexture != nullptr)
  {
    this->RGBATexture->Delete();
    this->RGBATexture = nullptr;
  }
  if (this->RootTexture != nullptr)
  {
    this->RootTexture->Delete();
    this->RootTexture = nullptr;
  }
}
