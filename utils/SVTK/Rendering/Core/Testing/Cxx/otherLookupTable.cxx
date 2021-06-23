/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the LookupTable

#include "svtkDebugLeaks.h"
#include "svtkLogLookupTable.h"
#include "svtkLookupTable.h"

void TestOLT(svtkLookupTable* lut1)
{
  // actual test

  lut1->SetRange(1, 1024);

  lut1->Allocate(1024);
  lut1->SetRampToLinear();
  lut1->Build();

  double rgb[4];
  lut1->GetColor(0, rgb);

  lut1->GetOpacity(0);

  lut1->GetTableValue(10, rgb);
  lut1->GetTableValue(10);

  unsigned char output[4 * 1024];

  int bitA = 1;
  lut1->MapScalarsThroughTable2(&bitA, output, SVTK_BIT, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(&bitA, output, SVTK_CHAR, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(&bitA, output, SVTK_CHAR, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(&bitA, output, SVTK_CHAR, 2, 1, SVTK_LUMINANCE);

  char charA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(charA, output, SVTK_CHAR, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(charA, output, SVTK_CHAR, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(charA, output, SVTK_CHAR, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(charA, output, SVTK_CHAR, 2, 1, SVTK_LUMINANCE);

  unsigned char ucharA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(ucharA, output, SVTK_UNSIGNED_CHAR, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(ucharA, output, SVTK_UNSIGNED_CHAR, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(ucharA, output, SVTK_UNSIGNED_CHAR, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(ucharA, output, SVTK_UNSIGNED_CHAR, 2, 1, SVTK_LUMINANCE);

  int intA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(intA, output, SVTK_INT, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(intA, output, SVTK_INT, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(intA, output, SVTK_INT, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(intA, output, SVTK_INT, 2, 1, SVTK_LUMINANCE);

  unsigned int uintA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(uintA, output, SVTK_UNSIGNED_INT, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(uintA, output, SVTK_UNSIGNED_INT, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(uintA, output, SVTK_UNSIGNED_INT, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(uintA, output, SVTK_UNSIGNED_INT, 2, 1, SVTK_LUMINANCE);

  long longA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(longA, output, SVTK_LONG, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(longA, output, SVTK_LONG, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(longA, output, SVTK_LONG, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(longA, output, SVTK_LONG, 2, 1, SVTK_LUMINANCE);

  unsigned long ulongA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(ulongA, output, SVTK_UNSIGNED_LONG, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(ulongA, output, SVTK_UNSIGNED_LONG, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(ulongA, output, SVTK_UNSIGNED_LONG, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(ulongA, output, SVTK_UNSIGNED_LONG, 2, 1, SVTK_LUMINANCE);

  short shortA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(shortA, output, SVTK_SHORT, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(shortA, output, SVTK_SHORT, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(shortA, output, SVTK_SHORT, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(shortA, output, SVTK_SHORT, 2, 1, SVTK_LUMINANCE);

  unsigned short ushortA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(ushortA, output, SVTK_UNSIGNED_SHORT, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(ushortA, output, SVTK_UNSIGNED_SHORT, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(ushortA, output, SVTK_UNSIGNED_SHORT, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(ushortA, output, SVTK_UNSIGNED_SHORT, 2, 1, SVTK_LUMINANCE);

  float floatA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(floatA, output, SVTK_FLOAT, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(floatA, output, SVTK_FLOAT, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(floatA, output, SVTK_FLOAT, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(floatA, output, SVTK_FLOAT, 2, 1, SVTK_LUMINANCE);

  double doubleA[2] = { 1, 10 };
  lut1->MapScalarsThroughTable2(doubleA, output, SVTK_DOUBLE, 2, 1, SVTK_RGBA);
  lut1->MapScalarsThroughTable2(doubleA, output, SVTK_DOUBLE, 2, 1, SVTK_RGB);
  lut1->MapScalarsThroughTable2(doubleA, output, SVTK_DOUBLE, 2, 1, SVTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(doubleA, output, SVTK_DOUBLE, 2, 1, SVTK_LUMINANCE);
}

int otherLookupTable(int, char*[])
{
  svtkLookupTable* lut1 = svtkLookupTable::New();
  lut1->SetAlpha(1.0);
  lut1->SetScaleToLinear();
  TestOLT(lut1);
  lut1->SetAlpha(.5);
  TestOLT(lut1);
  lut1->Delete();

  svtkLogLookupTable* lut2 = svtkLogLookupTable::New();
  lut2->SetAlpha(1.0);
  lut2->SetScaleToLog10();
  TestOLT(lut2);
  lut2->SetAlpha(.5);
  TestOLT(lut2);
  lut2->Delete();

  return 0;
}
