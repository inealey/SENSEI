/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkViewNodeFactory.h"
#include "svtkObjectFactory.h"
#include "svtkViewNode.h"

#include <map>
#include <string>

//============================================================================
class svtkViewNodeFactory::svtkInternals
{
public:
  std::map<std::string, svtkViewNode* (*)()> Overrides;

  svtkInternals() {}

  ~svtkInternals() { this->Overrides.clear(); }
};

//============================================================================
svtkStandardNewMacro(svtkViewNodeFactory);

//----------------------------------------------------------------------------
svtkViewNodeFactory::svtkViewNodeFactory()
{
  this->Internals = new svtkInternals;
}

//----------------------------------------------------------------------------
svtkViewNodeFactory::~svtkViewNodeFactory()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void svtkViewNodeFactory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNodeFactory::CreateNode(svtkObject* who)
{
  if (!who)
  {
    return nullptr;
  }

  svtkViewNode* (*func)() = nullptr;

  // First, check if there is an exact match for override functions for this
  // object type.
  {
    auto fnOverrideIt = this->Internals->Overrides.find(who->GetClassName());
    if (fnOverrideIt != this->Internals->Overrides.end())
    {
      func = fnOverrideIt->second;
    }
  }

  // Next, check if there is an indirect match (one of the parents of this
  // object type has an override). If there is more than one override for
  // types in this object's hierarchy, choose the most derived one.
  if (func == nullptr)
  {
    svtkIdType closest = SVTK_ID_MAX;
    for (auto it = this->Internals->Overrides.begin(); it != this->Internals->Overrides.end(); ++it)
    {
      svtkIdType numberOfGenerations = who->GetNumberOfGenerationsFromBase(it->first.c_str());
      if (numberOfGenerations >= 0 && numberOfGenerations < closest)
      {
        closest = numberOfGenerations;
        func = it->second;
      }
    }
  }

  // If neither are available, do not create a node for this object.
  if (func == nullptr)
  {
    return nullptr;
  }

  // Otherwise, create a node and initialize it.
  svtkViewNode* vn = func();
  vn->SetMyFactory(this);
  if (vn)
  {
    vn->SetRenderable(who);
  }

  return vn;
}

//----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
svtkViewNode* svtkViewNodeFactory::CreateNode(const char* forwhom)
{
  if (this->Internals->Overrides.find(forwhom) == this->Internals->Overrides.end())
  {
    return nullptr;
  }
  svtkViewNode* (*func)() = this->Internals->Overrides.find(forwhom)->second;
  svtkViewNode* vn = func();
  vn->SetMyFactory(this);
  return vn;
}
#endif

//----------------------------------------------------------------------------
void svtkViewNodeFactory::RegisterOverride(const char* name, svtkViewNode* (*func)())
{
  this->Internals->Overrides[name] = func;
}
