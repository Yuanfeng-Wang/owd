/***********************************************************************

  Copyright 2009-2010 Carnegie Mellon University and Intel Corporation
  Author: Mike Vande Weghe <vandeweg@cmu.edu>

  This file is part of owd.

  owd is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 3 of the License, or (at your
  option) any later version.

  owd is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ***********************************************************************/

#include <iostream>
#include <math.h>
#include "Trajectory.hh"
#include <vector>

#ifndef __SERVOTRAJ_HH__
#define __SERVOTRAJ_HH__

class ServoTraj : public Trajectory {
private:
  int nDOF;
  std::vector<double> stoptime;
  std::vector<double> target_velocity;
  std::vector<double> current_velocity;
  const float vlimit;
  const float accel;
  double jlimit_buffer;
  JointPos current_position;
  double lower_jlimit[7], upper_jlimit[7];

public:

  ServoTraj(int DOF, int id, double *start_pos,
	    double *lower_joint_limits = NULL,
	    double *upper_joint_limits = NULL);
  virtual ~ServoTraj();

  bool SetVelocity(int j, float v, float duration = 0.5);
  void stop(); // override of base class stop()
  
  // mandatory functions inherited from Trajectory
  void evaluate(double y[], double yd[], double ydd[], double dt);
};

#endif
