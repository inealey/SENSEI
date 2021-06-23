/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderTimer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLRenderTimer
 * @brief   Asynchronously measures GPU execution time for a single event.
 *
 *
 * This class posts events to the OpenGL server to measure execution times
 * of GPU processes. The queries are asynchronous and multiple
 * svtkOpenGLRenderTimers may overlap / be nested.
 *
 * This uses GL_TIMESTAMP rather than GL_ELAPSED_TIME, since only one
 * GL_ELAPSED_TIME query may be active at a time. Since GL_TIMESTAMP is not
 * available on OpenGL ES, timings will not be available on those platforms.
 * Use the static IsSupported() method to determine if the timer is available.
 */

#ifndef svtkOpenGLRenderTimer_h
#define svtkOpenGLRenderTimer_h

#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkType.h"                   // For svtkTypeUint64, etc

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLRenderTimer
{
public:
  svtkOpenGLRenderTimer();
  ~svtkOpenGLRenderTimer();

  /**
   * Returns true if timer events are supported by the current OpenGL
   * implementation.
   */
  static bool IsSupported();

  /**
   * Clear out any previous results and prepare for a new query.
   */
  void Reset();

  /**
   * Mark the start of a timed event.
   */
  void Start();

  /**
   * Mark the end of a timed event.
   */
  void Stop();

  /**
   * Returns true if the timer has been started. The query may not be ready yet.
   */
  bool Started();

  /**
   * Returns true if the timer has been stopped. The query may not be ready yet.
   */
  bool Stopped();

  /**
   * Returns true when the timing results are available.
   */
  bool Ready();

  //@{
  /**
   * If Ready() returns true, get the elapsed time in the requested units.
   */
  float GetElapsedSeconds();
  float GetElapsedMilliseconds();
  svtkTypeUInt64 GetElapsedNanoseconds();
  //@}

  //@{
  /**
   * This class can also be used in a reusable manner where the start and stop
   * events stay in flight until they are both completed. Calling ReusableStart
   * while they are in flight is ignored. The Elapsed time is always the result
   * from the most recently completed flight. Typical usage is
   *
   * render loop
   *   timer->ReusableStart();
   *   // do some rendering
   *   timer->ReusableStop();
   *   time = timer->GetReusableElapsedSeconds();
   *
   * the elapsed seconds will return zero until a flight has completed.
   *
   * The idea being that with OpenGL render commands are
   * asynchronous. You might render multiple times before the first
   * render on the GPU is completed. These reusable methods provide
   * a method for provinding a constant measure of the time required
   * for a command with the efficiency of only having one timing in
   * process/flight at a time. Making this a lightweight timer
   * in terms of OpenGL API calls.
   *
   * These reusable methods are not meant to be mixed with other methods
   * in this class.
   */
  void ReusableStart();
  void ReusableStop();
  float GetReusableElapsedSeconds();
  //@}

  /**
   * If Ready() returns true, return the start or stop time in nanoseconds.
   * @{
   */
  svtkTypeUInt64 GetStartTime();
  svtkTypeUInt64 GetStopTime();
  /**@}*/

  /**
   * Simply calls Reset() to ensure that query ids are freed. All stored timing
   * information will be lost.
   */
  void ReleaseGraphicsResources();

protected:
  bool StartReady;
  bool EndReady;

  svtkTypeUInt32 StartQuery;
  svtkTypeUInt32 EndQuery;

  svtkTypeUInt64 StartTime;
  svtkTypeUInt64 EndTime;

  bool ReusableStarted;
  bool ReusableEnded;

private:
  svtkOpenGLRenderTimer(const svtkOpenGLRenderTimer&) = delete;
  void operator=(const svtkOpenGLRenderTimer&) = delete;
};

#endif // svtkOpenGLRenderTimer_h

// SVTK-HeaderTest-Exclude: svtkOpenGLRenderTimer.h
