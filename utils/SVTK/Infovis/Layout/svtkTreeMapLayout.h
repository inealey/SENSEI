/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeMapLayout.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkTreeMapLayout
 * @brief   layout a svtkTree into a tree map
 *
 *
 * svtkTreeMapLayout assigns rectangular regions to each vertex in the tree,
 * creating a tree map.  The data is added as a data array with four
 * components per tuple representing the location and size of the
 * rectangle using the format (Xmin, Xmax, Ymin, Ymax).
 *
 * This algorithm relies on a helper class to perform the actual layout.
 * This helper class is a subclass of svtkTreeMapLayoutStrategy.
 *
 * @par Thanks:
 * Thanks to Brian Wylie and Ken Moreland from Sandia National Laboratories
 * for help developing this class.
 *
 * @par Thanks:
 * Tree map concept comes from:
 * Shneiderman, B. 1992. Tree visualization with tree-maps: 2-d space-filling approach.
 * ACM Trans. Graph. 11, 1 (Jan. 1992), 92-99.
 */

#ifndef svtkTreeMapLayout_h
#define svtkTreeMapLayout_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class svtkTreeMapLayoutStrategy;

class SVTKINFOVISLAYOUT_EXPORT svtkTreeMapLayout : public svtkTreeAlgorithm
{
public:
  static svtkTreeMapLayout* New();

  svtkTypeMacro(svtkTreeMapLayout, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The field name to use for storing the rectangles for each vertex.
   * The rectangles are stored in a quadruple float array
   * (minX, maxX, minY, maxY).
   */
  svtkGetStringMacro(RectanglesFieldName);
  svtkSetStringMacro(RectanglesFieldName);
  //@}

  /**
   * The array to use for the size of each vertex.
   */
  virtual void SetSizeArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  //@{
  /**
   * The strategy to use when laying out the tree map.
   */
  svtkGetObjectMacro(LayoutStrategy, svtkTreeMapLayoutStrategy);
  void SetLayoutStrategy(svtkTreeMapLayoutStrategy* strategy);
  //@}

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it)
   */
  svtkIdType FindVertex(float pnt[2], float* binfo = nullptr);

  /**
   * Return the min and max 2D points of the
   * vertex's bounding box
   */
  void GetBoundingBox(svtkIdType id, float* binfo);

  /**
   * Get the modification time of the layout algorithm.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkTreeMapLayout();
  ~svtkTreeMapLayout() override;

  char* RectanglesFieldName;
  svtkTreeMapLayoutStrategy* LayoutStrategy;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTreeMapLayout(const svtkTreeMapLayout&) = delete;
  void operator=(const svtkTreeMapLayout&) = delete;
};

#endif
