/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextClip
 * @brief   all children of this item are clipped
 * by the specified area.
 *
 *
 * This class can be used to clip the rendering of an item inside a rectangular
 * area.
 */

#ifndef svtkContextClip_h
#define svtkContextClip_h

#include "svtkAbstractContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkSmartPointer.h"             // Needed for SP ivars.

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextClip : public svtkAbstractContextItem
{
public:
  svtkTypeMacro(svtkContextClip, svtkAbstractContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a svtkContextClip object.
   */
  static svtkContextClip* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Set the origin, width and height of the clipping rectangle. These are in
   * pixel coordinates.
   */
  virtual void SetClip(float x, float y, float width, float height);

  /**
   * Get the clipping rectangle parameters in pixel coordinates:
   */
  virtual void GetRect(float rect[4]);
  virtual float GetX() { return Dims[0]; }
  virtual float GetY() { return Dims[1]; }
  virtual float GetWidth() { return Dims[2]; }
  virtual float GetHeight() { return Dims[3]; }

protected:
  svtkContextClip();
  ~svtkContextClip() override;

  float Dims[4];

private:
  svtkContextClip(const svtkContextClip&) = delete;
  void operator=(const svtkContextClip&) = delete;
};

inline void svtkContextClip::GetRect(float rect[4])
{
  rect[0] = this->Dims[0];
  rect[1] = this->Dims[1];
  rect[2] = this->Dims[2];
  rect[3] = this->Dims[3];
}

#endif // svtkContextClip_h
