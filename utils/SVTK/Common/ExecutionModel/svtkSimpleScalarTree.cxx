/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleScalarTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimpleScalarTree.h"

#include "svtkCell.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

class svtkScalarNode
{
};

template <class TScalar>
class svtkScalarRange : public svtkScalarNode
{
public:
  TScalar min;
  TScalar max;
};

//---The SVTK Classes proper------------------------------------------------------

svtkStandardNewMacro(svtkSimpleScalarTree);

//-----------------------------------------------------------------------------
// Instantiate scalar tree with maximum level of 20 and branching
// factor of 3.
svtkSimpleScalarTree::svtkSimpleScalarTree()
{
  this->MaxLevel = 20;
  this->Level = 0;
  this->BranchingFactor = 3;
  this->Tree = nullptr;
  this->TreeSize = 0;

  // Variables that support serial traversal
  this->NumCells = 0;
  this->TreeIndex = 0;
  this->ChildNumber = 0;
  this->CellId = 0;

  // Variable for parallel traversal
  this->CandidateCells = nullptr;
  this->NumCandidates = 0;
}

//-----------------------------------------------------------------------------
svtkSimpleScalarTree::~svtkSimpleScalarTree()
{
  delete[] this->Tree;
}

//-----------------------------------------------------------------------------
// Shallow copy enough information for a clone to produce the same result on
// the same data.
void svtkSimpleScalarTree::ShallowCopy(svtkScalarTree* stree)
{
  svtkSimpleScalarTree* sst = svtkSimpleScalarTree::SafeDownCast(stree);
  if (sst != nullptr)
  {
    this->SetMaxLevel(sst->GetMaxLevel());
    this->SetBranchingFactor(sst->GetBranchingFactor());
  }
  // Now do superclass
  this->Superclass::ShallowCopy(stree);
}

//-----------------------------------------------------------------------------
// Initialize locator. Frees memory and resets object as appropriate.
void svtkSimpleScalarTree::Initialize()
{
  delete[] this->Tree;
  this->Tree = nullptr;
}

//-----------------------------------------------------------------------------
// Construct the scalar tree from the dataset provided. Checks build times
// and modified time from input and reconstructs the tree if necessary.
void svtkSimpleScalarTree::BuildTree()
{
  svtkIdType cellId, i, j, numScalars;
  int level, offset, parentOffset, prod;
  svtkIdType numNodes, node, numLeafs, leaf, numParentLeafs;
  svtkCell* cell;
  svtkIdList* cellPts;
  svtkScalarRange<double>*tree, *parent;
  double* s;
  svtkDoubleArray* cellScalars;

  // Check input...see whether we have to rebuild
  //
  if (!this->DataSet || (this->NumCells = this->DataSet->GetNumberOfCells()) < 1)
  {
    svtkErrorMacro(<< "No data to build tree with");
    return;
  }

  if (this->Tree != nullptr && this->BuildTime > this->MTime &&
    this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }

  svtkDebugMacro(<< "Building scalar tree...");

  // If no scalars set then try and grab them from dataset
  if (!this->Scalars)
  {
    this->SetScalars(this->DataSet->GetPointData()->GetScalars());
  }
  if (!this->Scalars)
  {
    svtkErrorMacro(<< "No scalar data to build trees with");
    return;
  }

  this->Initialize();
  cellScalars = svtkDoubleArray::New();
  cellScalars->Allocate(100);

  // Compute the number of levels in the tree
  //
  numLeafs = static_cast<int>(ceil(static_cast<double>(this->NumCells) / this->BranchingFactor));
  for (prod = 1, numNodes = 1, this->Level = 0; prod < numLeafs && this->Level <= this->MaxLevel;
       this->Level++)
  {
    prod *= this->BranchingFactor;
    numNodes += prod;
  }

  this->LeafOffset = offset = numNodes - prod;
  svtkScalarRange<double>* TTree;
  this->TreeSize = numNodes - (prod - numLeafs);
  this->Tree = TTree = new svtkScalarRange<double>[this->TreeSize];
  for (i = 0; i < this->TreeSize; i++)
  {
    TTree[i].min = SVTK_DOUBLE_MAX;
    TTree[i].max = -SVTK_DOUBLE_MAX;
  }

  // Loop over all cells getting range of scalar data and place into leafs
  //
  for (cellId = 0, node = 0; node < numLeafs; node++)
  {
    tree = TTree + offset + node;
    for (i = 0; i < this->BranchingFactor && cellId < this->NumCells; i++, cellId++)
    {
      cell = this->DataSet->GetCell(cellId);
      cellPts = cell->GetPointIds();
      numScalars = cellPts->GetNumberOfIds();
      cellScalars->SetNumberOfTuples(numScalars);
      this->Scalars->GetTuples(cellPts, cellScalars);
      s = cellScalars->GetPointer(0);

      for (j = 0; j < numScalars; j++)
      {
        if (s[j] < tree->min)
        {
          tree->min = s[j];
        }
        if (s[j] > tree->max)
        {
          tree->max = s[j];
        }
      }
    }
  }

  // Now build top levels of tree in bottom-up fashion
  //
  for (level = this->Level; level > 0; level--)
  {
    parentOffset = offset - prod / this->BranchingFactor;
    prod /= this->BranchingFactor;
    numParentLeafs = static_cast<int>(ceil(static_cast<double>(numLeafs) / this->BranchingFactor));

    for (leaf = 0, node = 0; node < numParentLeafs; node++)
    {
      parent = TTree + parentOffset + node;
      for (i = 0; i < this->BranchingFactor && leaf < numLeafs; i++, leaf++)
      {
        tree = TTree + offset + leaf;
        if (tree->min < parent->min)
        {
          parent->min = tree->min;
        }
        if (tree->max > parent->max)
        {
          parent->max = tree->max;
        }
      }
    }

    numLeafs = numParentLeafs;
    offset = parentOffset;
  }

  this->BuildTime.Modified();
  cellScalars->Delete();
}

//-----------------------------------------------------------------------------
// Begin to traverse the cells based on a scalar value. Returned cells
// will have scalar values that span the scalar value specified.
void svtkSimpleScalarTree::InitTraversal(double scalarValue)
{
  this->BuildTree();
  svtkScalarRange<double>* TTree = static_cast<svtkScalarRange<double>*>(this->Tree);

  this->ScalarValue = scalarValue;
  this->TreeIndex = this->TreeSize;

  // Check root of tree for overlap with scalar value
  //
  if (TTree[0].min > scalarValue || TTree[0].max < scalarValue)
  {
    return;
  }

  else // find leaf that overlaps the isocontour value
  {
    this->FindStartLeaf(0, 0); // recursive function call
  }
}

//-----------------------------------------------------------------------------
int svtkSimpleScalarTree::FindStartLeaf(svtkIdType index, int level)
{
  if (level < this->Level)
  {
    int i;
    svtkIdType childIndex = this->BranchingFactor * index + 1;

    level++;
    for (i = 0; i < this->BranchingFactor; i++)
    {
      index = childIndex + i;
      if (index >= this->TreeSize)
      {
        this->TreeIndex = this->TreeSize;
        return 0;
      }
      else if (this->FindStartLeaf(childIndex + i, level))
      {
        return 1;
      }
    }

    return 0;
  }

  else // recursion terminated
  {
    svtkScalarRange<double>* tree = static_cast<svtkScalarRange<double>*>(this->Tree) + index;

    if (tree->min > this->ScalarValue || tree->max < this->ScalarValue)
    {
      return 0;
    }
    else
    {
      this->ChildNumber = 0;
      this->TreeIndex = index;
      this->CellId = (index - this->LeafOffset) * this->BranchingFactor;
      return 1;
    }
  }
}

//-----------------------------------------------------------------------------
int svtkSimpleScalarTree::FindNextLeaf(svtkIdType childIndex, int childLevel)
{
  svtkIdType myIndex = (childIndex - 1) / this->BranchingFactor;
  int myLevel = childLevel - 1;
  svtkIdType firstChildIndex, childNum, index;

  // Find which child invoked this method
  firstChildIndex = myIndex * this->BranchingFactor + 1;
  childNum = childIndex - firstChildIndex;

  for (childNum++; childNum < this->BranchingFactor; childNum++)
  {
    index = firstChildIndex + childNum;
    if (index >= this->TreeSize)
    {
      this->TreeIndex = this->TreeSize;
      return 0;
    }
    else if (this->FindStartLeaf(index, childLevel))
    {
      return 1;
    }
  }

  // If here, didn't find anything yet
  if (myLevel <= 0) // at root, can't go any higher in tree
  {
    this->TreeIndex = this->TreeSize;
    return 0;
  }
  else
  {
    return this->FindNextLeaf(myIndex, myLevel);
  }
}

//-----------------------------------------------------------------------------
// Return the next cell that may contain scalar value specified to
// initialize traversal. The value nullptr is returned if the list is
// exhausted. Make sure that InitTraversal() has been invoked first or
// you'll get erratic behavior.
svtkCell* svtkSimpleScalarTree::GetNextCell(
  svtkIdType& cellId, svtkIdList*& cellPts, svtkDataArray* cellScalars)
{
  double s, min = SVTK_DOUBLE_MAX, max = (-SVTK_DOUBLE_MAX);
  svtkIdType i, numScalars;
  svtkCell* cell;
  svtkIdType numCells = this->NumCells;

  while (this->TreeIndex < this->TreeSize)
  {
    for (; this->ChildNumber < this->BranchingFactor && this->CellId < numCells;
         this->ChildNumber++, this->CellId++)
    {
      cell = this->DataSet->GetCell(this->CellId);
      cellPts = cell->GetPointIds();
      numScalars = cellPts->GetNumberOfIds();
      cellScalars->SetNumberOfTuples(numScalars);
      this->Scalars->GetTuples(cellPts, cellScalars);
      for (i = 0; i < numScalars; i++)
      {
        s = cellScalars->GetTuple1(i);
        if (s < min)
        {
          min = s;
        }
        if (s > max)
        {
          max = s;
        }
      }
      if (this->ScalarValue >= min && this->ScalarValue <= max)
      {
        cellId = this->CellId;
        this->ChildNumber++; // prepare for next time
        this->CellId++;
        return cell;
      }
    } // for each cell in this leaf

    // If here, must have not found anything in this leaf
    this->FindNextLeaf(this->TreeIndex, this->Level);
  } // while not all leafs visited

  return nullptr;
}

//-----------------------------------------------------------------------------
// Return the number of cell batches.

//-----------------------------------------------------------------------------
// Return the number of chunks of data that can be iterated over.
svtkIdType svtkSimpleScalarTree::GetNumberOfCellBatches(double scalarValue)
{
  // Modified time prevents rebuilding
  this->BuildTree();
  svtkScalarRange<double>* TTree = static_cast<svtkScalarRange<double>*>(this->Tree);

  this->ScalarValue = scalarValue;
  this->TreeIndex = this->TreeSize;

  // Check root of tree for overlap with scalar value
  //
  if (TTree[0].min > scalarValue || TTree[0].max < scalarValue)
  {
    return 0;
  }

  // Basically we do a traversal of the tree and identify potential candidates.
  // It is essential that InitTraversal() has been called first.
  this->NumCandidates = 0;
  if (this->CandidateCells)
  {
    delete[] this->CandidateCells;
    this->CandidateCells = nullptr;
  }
  if (this->NumCells < 1)
  {
    return 0;
  }
  this->CandidateCells = new svtkIdType[this->NumCells];

  // Now begin traversing tree. InitTraversal() sets the first CellId.
  while (this->TreeIndex < this->TreeSize)
  {
    for (; this->ChildNumber < this->BranchingFactor && this->CellId < this->NumCells;
         this->ChildNumber++, this->CellId++)
    {
      this->CandidateCells[this->NumCandidates++] = this->CellId;
    } // for each cell in this leaf

    // If here, must have not found anything in this leaf
    this->FindNextLeaf(this->TreeIndex, this->Level);
  } // while not all leafs visited

  // Watch for boundary conditions
  if (this->NumCandidates < 1)
  {
    return 0;
  }
  else
  {
    return (((this->NumCandidates - 1) / this->BranchingFactor) + 1);
  }
}

//-----------------------------------------------------------------------------
// Return the number of chunks of data that can be iterated over.
const svtkIdType* svtkSimpleScalarTree::GetCellBatch(svtkIdType batchNum, svtkIdType& numCells)
{
  // Make sure that everything is hunky dory
  svtkIdType pos = batchNum * this->BranchingFactor;
  if (this->NumCells < 1 || !this->CandidateCells || pos > this->NumCandidates)
  {
    numCells = 0;
    return nullptr;
  }

  if ((this->NumCandidates - pos) >= this->BranchingFactor)
  {
    numCells = this->BranchingFactor;
  }
  else
  {
    numCells = this->NumCandidates % this->BranchingFactor;
  }
  return this->CandidateCells + pos;
}

//-----------------------------------------------------------------------------
void svtkSimpleScalarTree::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Level: " << this->GetLevel() << "\n";
  os << indent << "Max Level: " << this->GetMaxLevel() << "\n";
  os << indent << "Branching Factor: " << this->GetBranchingFactor() << "\n";
}
