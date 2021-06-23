/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPContourGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSMPContourGrid
 * @brief   a subclass of svtkContourGrid that works in parallel
 * svtkSMPContourGrid performs the same functionaliy as svtkContourGrid but does
 * it using multiple threads. This will probably be merged with svtkContourGrid
 * in the future.
 */

#ifndef svtkSMPContourGrid_h
#define svtkSMPContourGrid_h

#include "svtkContourGrid.h"
#include "svtkFiltersSMPModule.h" // For export macro

class SVTKFILTERSSMP_EXPORT svtkSMPContourGrid : public svtkContourGrid
{
public:
  svtkTypeMacro(svtkSMPContourGrid, svtkContourGrid);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Constructor.
   */
  static svtkSMPContourGrid* New();

  //@{
  /**
   * If MergePieces is true (default), this filter will merge all
   * pieces generated by processing the input with multiple threads.
   * The output will be a svtkPolyData. Note that this has a slight overhead
   * which becomes more significant as the number of threads used grows.
   * If MergePieces is false, this filter will generate a svtkMultiBlock
   * of svtkPolyData where the number of pieces will be equal to the number
   * of threads used.
   */
  svtkSetMacro(MergePieces, bool);
  svtkGetMacro(MergePieces, bool);
  svtkBooleanMacro(MergePieces, bool);
  //@}

  /**
   * Please see svtkAlgorithm for details.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkSMPContourGrid();
  ~svtkSMPContourGrid() override;

  virtual int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  bool MergePieces;

private:
  svtkSMPContourGrid(const svtkSMPContourGrid&) = delete;
  void operator=(const svtkSMPContourGrid&) = delete;
};

#endif
