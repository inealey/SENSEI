/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayWriter.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkArrayWriter
 * @brief   Serialize sparse and dense arrays to a file or stream.
 *
 *
 * svtkArrayWriter serializes sparse and dense array data using a text-based
 * format that is human-readable and easily parsed (default option).  The
 * WriteBinary array option can be used to serialize the sparse and dense array data
 * using a binary format that is optimized for rapid throughput.
 *
 * svtkArrayWriter can be used in two distinct ways: first, it can be used as a
 * normal pipeline filter, which writes its inputs to a file.  Alternatively, static
 * methods are provided for writing svtkArray instances to files or arbitrary c++
 * streams.
 *
 * Inputs:
 *   Input port 0: (required) svtkArrayData object containing a single sparse or dense
 *                            array.
 *
 * Output Format:
 *   See http://www.kitware.com/InfovisWiki/index.php/N-Way_Array_File_Formats for
 *   details on how svtkArrayWriter encodes data.
 *
 * @sa
 * svtkArrayReader
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkArrayWriter_h
#define svtkArrayWriter_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkStdString.h"    // For string API
#include "svtkWriter.h"

class svtkArray;

class SVTKIOCORE_EXPORT svtkArrayWriter : public svtkWriter
{
public:
  static svtkArrayWriter* New();
  svtkTypeMacro(svtkArrayWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get / set the filename where data will be stored (when used as a filter).
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get / set whether data will be written in binary format (when used as a filter).
   */
  svtkSetMacro(Binary, svtkTypeBool);
  svtkGetMacro(Binary, svtkTypeBool);
  svtkBooleanMacro(Binary, svtkTypeBool);
  //@}

  /**
   * The output string. This is only set when WriteToOutputString is set.
   */
  virtual svtkStdString GetOutputString() { return this->OutputString; }

  //@{
  /**
   * Whether to output to a string instead of to a file, which is the default.
   */
  svtkSetMacro(WriteToOutputString, bool);
  svtkGetMacro(WriteToOutputString, bool);
  svtkBooleanMacro(WriteToOutputString, bool);
  //@}

  int Write() override; // This is necessary to get Write() wrapped for scripting languages.

  /**
   * Writes input port 0 data to a file, using an arbitrary filename and binary flag.
   */
  bool Write(const svtkStdString& FileName, bool WriteBinary = false);

  /**
   * Write an arbitrary array to a file, without using the pipeline.
   */
  static bool Write(svtkArray* array, const svtkStdString& file_name, bool WriteBinary = false);

  /**
   * Write input port 0 data to an arbitrary stream.  Note: streams should always be opened in
   * binary mode, to prevent problems reading files on Windows.
   */
  bool Write(ostream& stream, bool WriteBinary = false);

  /**
   * Write arbitrary data to a stream without using the pipeline.  Note: streams should always
   * be opened in binary mode, to prevent problems reading files on Windows.
   */
  static bool Write(svtkArray* array, ostream& stream, bool WriteBinary = false);

  /**
   * Write input port 0 data to a string. Note that the WriteBinary argument is not
   * optional in order to not clash with the inherited Write() method.
   */
  svtkStdString Write(bool WriteBinary);

  /**
   * Write arbitrary data to a string without using the pipeline.
   */
  static svtkStdString Write(svtkArray* array, bool WriteBinary = false);

protected:
  svtkArrayWriter();
  ~svtkArrayWriter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  void WriteData() override;

  char* FileName;
  svtkTypeBool Binary;
  bool WriteToOutputString;
  svtkStdString OutputString;

private:
  svtkArrayWriter(const svtkArrayWriter&) = delete;
  void operator=(const svtkArrayWriter&) = delete;
};

#endif
