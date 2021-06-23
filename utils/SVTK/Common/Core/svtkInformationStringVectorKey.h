/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationStringVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInformationStringVectorKey
 * @brief   Key for String vector values.
 *
 * svtkInformationStringVectorKey is used to represent keys for String
 * vector values in svtkInformation.h
 */

#ifndef svtkInformationStringVectorKey_h
#define svtkInformationStringVectorKey_h

#include "svtkCommonCoreModule.h" // For export macro
#include "svtkInformationKey.h"

#include "svtkCommonInformationKeyManager.h" // Manage instances of this type.

#include <string> // for std::string compat

class SVTKCOMMONCORE_EXPORT svtkInformationStringVectorKey : public svtkInformationKey
{
public:
  svtkTypeMacro(svtkInformationStringVectorKey, svtkInformationKey);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkInformationStringVectorKey(const char* name, const char* location, int length = -1);
  ~svtkInformationStringVectorKey() override;

  /**
   * This method simply returns a new svtkInformationStringVectorKey, given a
   * name, a location and a required length. This method is provided for
   * wrappers. Use the constructor directly from C++ instead.
   */
  static svtkInformationStringVectorKey* MakeKey(
    const char* name, const char* location, int length = -1)
  {
    return new svtkInformationStringVectorKey(name, location, length);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(svtkInformation* info, const char* value);
  void Set(svtkInformation* info, const char* value, int idx = 0);
  void Append(svtkInformation* info, const std::string& value);
  void Set(svtkInformation* info, const std::string& value, int idx = 0);
  const char* Get(svtkInformation* info, int idx = 0);
  int Length(svtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(svtkInformation* from, svtkInformation* to) override;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, svtkInformation* info) override;

protected:
  // The required length of the vector value (-1 is no restriction).
  int RequiredLength;

private:
  svtkInformationStringVectorKey(const svtkInformationStringVectorKey&) = delete;
  void operator=(const svtkInformationStringVectorKey&) = delete;
};

#endif
