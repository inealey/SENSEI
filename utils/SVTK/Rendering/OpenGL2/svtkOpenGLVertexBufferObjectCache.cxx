/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVertexBufferObjectCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLVertexBufferObjectCache.h"

#include "svtkDataArray.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLVertexBufferObject.h"

// ----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLVertexBufferObjectCache);

// ----------------------------------------------------------------------------
svtkOpenGLVertexBufferObjectCache::svtkOpenGLVertexBufferObjectCache() = default;

// ----------------------------------------------------------------------------
svtkOpenGLVertexBufferObjectCache::~svtkOpenGLVertexBufferObjectCache() = default;

void svtkOpenGLVertexBufferObjectCache::RemoveVBO(svtkOpenGLVertexBufferObject* vbo)
{
  svtkOpenGLVertexBufferObjectCache::VBOMap::iterator iter = this->MappedVBOs.begin();
  while (iter != this->MappedVBOs.end())
  {
    if (iter->second == vbo)
    {
      iter->first->UnRegister(this);
      this->MappedVBOs.erase(iter++);
    }
    else
    {
      ++iter;
    }
  }
}

// ----------------------------------------------------------------------------
svtkOpenGLVertexBufferObject* svtkOpenGLVertexBufferObjectCache::GetVBO(
  svtkDataArray* array, int destType)
{
  // Check array is valid
  if (array == nullptr || array->GetNumberOfTuples() == 0)
  {
    svtkErrorMacro(<< "Cannot get VBO for empty array.");
    return nullptr;
  }

  // Look for VBO in map
  VBOMap::const_iterator iter = this->MappedVBOs.find(array);
  if (iter != this->MappedVBOs.end())
  {
    svtkOpenGLVertexBufferObject* vbo = iter->second;
    vbo->SetDataType(destType);
    vbo->Register(this);
    return vbo;
  }

  // If vbo not found, create new one
  // Initialize new vbo
  svtkOpenGLVertexBufferObject* vbo = svtkOpenGLVertexBufferObject::New();
  vbo->SetCache(this);
  vbo->SetDataType(destType);
  array->Register(this);

  // Add vbo to map
  this->MappedVBOs[array] = vbo;
  return vbo;
}

// ----------------------------------------------------------------------------
void svtkOpenGLVertexBufferObjectCache::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
