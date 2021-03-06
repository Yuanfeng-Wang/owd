/***********************************************************************

  Copyright 2011 Carnegie Mellon University and Intel Corporation
  Author: Mike Vande Weghe <vandeweg@cmu.edu>

**********************************************************************/

#ifndef WSTRAJ_HH
#define WSTRAJ_HH

#include "HybridPlugin.h"
#include "ForceController.h"
#include "ApplyForceTraj.h"
#include <openwam/Trajectory.hh>
#include <openwam/ParabolicSegment.hh>
#include <owd_msgs/AddWSTraj.h>
#include <owd_msgs/Reset.h>


class WSTraj : public OWD::Trajectory {
public:

  /// Holds the endpoint at the current position while applying
  /// force in the specified direction.  Endpoint is free to move
  /// in the direction of the force
  WSTraj(owd_msgs::AddWSTraj::Request &wst);
  virtual ~WSTraj();

  virtual void evaluate_abs(OWD::Trajectory::TrajControl &tc, double t);
  virtual void force_feedback_evaluate(OWD::Trajectory::TrajControl &tc,
				       double dt);

  static bool AddWSTraj(owd_msgs::AddWSTraj::Request &req,
			owd_msgs::AddWSTraj::Response &res);

  static bool ForceFeedbackNextTrajSrv(owd_msgs::Reset::Request &req,
				       owd_msgs::Reset::Response &res);

  /// functions for starting up and shutting down the service
  static bool Register();
  static void Shutdown();

  double driving_forcetorque;
  R6 driving_force_direction;
  SE3 start_endpoint;
  R3 endpoint_translation;
  R3 movement_direction;
  so3 endpoint_rotation;
  OWD::JointPos joint_change;
  double max_linear_vel, max_linear_accel,
    max_angular_vel, max_angular_accel;
  OWD::ParabolicSegment parseg;
  double vel_factor;
  double force_scale;
  static const double force_scale_gain = 0.002;
  ForceController force_controller;
  ApplyForceTraj *AFTraj;
  bool ForceFeedback;

  /// Static members for handling the ROS service calls
  static ros::ServiceServer ss_AddWSTraj, ss_ForceFeedbackNextTraj;
  static bool ForceFeedbackNextTraj;
  
};

#endif // WSTRAJ_HH
