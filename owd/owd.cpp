/***********************************************************************

  Copyright 2008-2012 Carnegie Mellon University and Intel Corporation
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

#include <ros/ros.h>
#include <ros/time.h>
#include <owd_msgs/WAMState.h>
#include <owd_msgs/IndexedJointValues.h>
#include <owd_msgs/WamSetupSeaCtrl.h>
#include "openwamdriver.h"
#include "bhd280.hh"
#include "ft.hh"
#include "tactile.hh"
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <syscall.h>

int main(int argc, char** argv)
{
  ros::init(argc, argv, std::string("owd"));

#ifdef OWD_RT
  if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1){
    ROS_FATAL("owd: mlockall failed: ");
    return NULL;
  }
#else // OWD_RT
#ifndef OWDSIM
  // Set our RT scheduler profile so that we can get the read timeouts
  // we ask for from the Peak CANbus driver
  sched_param sp;
  sp.sched_priority=10;
  int priority_limit = sched_get_priority_min(SCHED_RR);
  if (priority_limit < 0) {
    ROS_FATAL("Could not retrieve minimum SCHED_RR priority level: %s",
	      strerror(errno));
    return 1;
  }
  if (sp.sched_priority < priority_limit) {
    ROS_FATAL("Unable to set desired RT scheduler priority: desired value of %d is below the minimum of %d",sp.sched_priority,priority_limit);
    return 1;
  }
  priority_limit = sched_get_priority_max(SCHED_RR);
  if (priority_limit < 0) {
    ROS_FATAL("Could not retrieve maximum SCHED_RR priority level: %s",
	      strerror(errno));
    return 1;
  }
  if (sp.sched_priority > priority_limit) {
    ROS_FATAL("Unable to set desired RT scheduler priority: desired value of %d is above the maximum of %d",sp.sched_priority,priority_limit);
    return 1;
  }
  int32_t err=sched_setscheduler(0,SCHED_RR, &sp);
  if (err) {
    if (errno == EPERM) {
      ROS_FATAL("Not permitted to set the RT scheduler priority for this process.  Please make sure the file /etc/security/limits.conf contains the line '* - rtprio unlimited' or run as root.");
      return 1;
    }
    ROS_FATAL("Could not set scheduler policy: %s",
	      strerror(errno));
    return 1;
  }
#endif // OWDSIM
#endif // OWD_RT


  // read parameters and set wam options

  ros::NodeHandle n("~");
  std::string calibration_filename;
  int canbus_number;
  std::string hand_type;
  bool forcetorque;
  bool modified_j1;
  int pub_freq;  // DEPRECATED
  int wam_pub_freq;
  int hand_pub_freq;
  int ft_pub_freq;
  int tactile_pub_freq;
  std::string cpu_list;

  n.param("calibration_file",calibration_filename,std::string("wam_joint_calibrations"));
  n.param("canbus_number",canbus_number,0);
  n.param("hand_type",hand_type,std::string("none"));
  n.param("forcetorque_sensor",forcetorque,false);
  n.param("modified_j1",modified_j1,false);
  int wam_freq_default=10;
  int hand_freq_default=10;
  if (n.getParam("publish_frequency",pub_freq)) {
    ROS_WARN("Parameter publish_frequency has been deprecated.  Please use wam_publish_frequency, hand_publish_frequency, ft_publish_frequency, and tactile_publish_frequency.");
    // we'll use the deprecated value to set the defaults for the
    // individual hand and wam frequencies, so that it will only be
    // used if the newer params are not specified
    wam_freq_default=hand_freq_default=pub_freq;
  }
  n.param("wam_publish_frequency",wam_pub_freq,wam_freq_default);
  n.param("hand_publish_frequency",hand_pub_freq,hand_freq_default);
  n.param("ft_publish_frequency",ft_pub_freq,10);
  n.param("tactile_publish_frequency",tactile_pub_freq,10);

  n.param("cpus_to_run_on",cpu_list,std::string(""));
  if (cpu_list.length() > 0) {
    // bind our thread to the specified processors
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    char *cpustr = strdup(cpu_list.c_str());
    char *cpustrtok(cpustr);
    char *cpu;
    char *strtokptr;
    char cpulistout[200];
    cpulistout[0] = 0;
    while ((cpu = strtok_r(cpustrtok,", ",&strtokptr))) {
      cpustrtok=NULL;
      int cpunum = atoi(cpu);
      CPU_SET(cpunum,&cpuset);
      snprintf(cpulistout + strlen(cpulistout), 200 - strlen(cpulistout), "%d ", cpunum);
    }
    free(cpustr);
    if (CPU_COUNT(&cpuset) > 0) {
      ROS_INFO("Restricting process to run on core(s) %s",cpulistout);
      pid_t tid = (pid_t) syscall (SYS_gettid);
      int err=sched_setaffinity(tid,sizeof(cpu_set_t),&cpuset);
      if (err) {
	ROS_FATAL("Could not bind our process to the preferred CPU cores: %s",
		  strerror(errno));
      }
    } else {
      ROS_ERROR("Could not add specified processor cores to the affinity set");
    }
  }


  if (wam_pub_freq > 500) {
    ROS_WARN("value of wam_publish_frequency exceeds maximum sensor rate; capping to 500Hz");
    wam_pub_freq = 500;
  }
  if (hand_pub_freq > 40) {
    ROS_WARN("value of hand_publish_frequency exceeds maximum sensor rate; capping to 40Hz");
    hand_pub_freq = 40;
  }
  if (ft_pub_freq > 500) {
    ROS_WARN("value of ft_publish_frequency exceeds maximum sensor rate; capping to 500Hz");
    ft_pub_freq = 500;
  }
  if (tactile_pub_freq > 40) {
    ROS_WARN("value of tactile_publish_frequency exceeds maximum sensor rate; capping to 40Hz");
    tactile_pub_freq = 40;
  }
  
  ROS_DEBUG("Using CANbus number %d",canbus_number);

  int BH_model(0);
  bool tactile(false);
  if (! hand_type.compare(0,3,"280")) {
    BH_model=280;
    if (! hand_type.compare("280+TACT")) {
      ROS_DEBUG("Expecting tactile sensors on this hand");
      tactile=true;
    }
  } else if (! hand_type.compare(0,3,"260")) {
    BH_model=260;
  } else if (! hand_type.compare(0,7,"Robotiq")) {
    BH_model=998;
  } else if (! hand_type.compare(0,6,"irobot")) {
    BH_model=978;
  } else if (! hand_type.compare(0,29,"darpa_arms_calibration_target")) {
    BH_model=999;
  } else if (hand_type.compare(0,4, "none")) { // note the absence of the !
    ROS_FATAL("Unknown hand type \"%s\"; cannot continue with unknown mass properties",
	      hand_type.c_str());
    exit(1);
  }

  OWD::WamDriver *wamdriver = new OWD::WamDriver(canbus_number,BH_model,forcetorque,tactile);
  if (modified_j1) {
    wamdriver->SetModifiedJ1(true);
  }
  try {
    if (! wamdriver->Init(calibration_filename.c_str())) {
      ROS_FATAL("WamDriver::Init() returned false; exiting.");
      delete wamdriver;
      exit(1);
    }
  } catch (int error) {
    ROS_FATAL("Error during WamDriver::Init(); exiting.");
    delete wamdriver;
    exit(1);
  } catch (const char *errmsg) {
    ROS_FATAL("Error during WamDriver::Init(): %s",errmsg);
    delete wamdriver;
    exit(1);
  }
#ifndef BH280_ONLY
  try {
    wamdriver->AdvertiseAndSubscribe(n);
  } catch (int error) {
    ROS_FATAL("Error during WamDriver::AdvertiseAndSubscribe(); exiting.");
    delete wamdriver;
    exit(1);
  }
#endif // BH280_ONLY
  
#ifndef OWDSIM
  BHD_280 *bhd(NULL);
  if (BH_model == 280) {
    bhd= new BHD_280(wamdriver->bus);
  }

  FT *ft(NULL);
  if (forcetorque) {
    ft = new FT(wamdriver->bus);
  }

  Tactile *tact(NULL);
  if (tactile) {
    tact = new Tactile(wamdriver->bus);
  }
#endif // OWDSIM


#ifdef BUILD_FOR_SEA
  usleep(100000);
  wamdriver->publishAllSeaSettings();
#endif // BUILD_FOR_SEA

  ROS_DEBUG("Creating timer and spinner threads");

#ifndef BH280_ONLY
  ros::Timer wam_timer = n.createTimer(ros::Rate(wam_pub_freq).expectedCycleTime(), &OWD::WamDriver::Pump, wamdriver);
#endif // BH280_ONLY

#ifndef OWDSIM
  ros::Timer bhd_timer;
  if (bhd) {
    bhd_timer = n.createTimer(ros::Rate(hand_pub_freq).expectedCycleTime(), &BHD_280::Pump, bhd);
  }
  ros::Timer ft_timer;
  if (ft) {
    ft_timer = n.createTimer(ros::Rate(ft_pub_freq).expectedCycleTime(), &FT::Pump, ft);
  }
  ros::Timer tactile_timer;
  if (tact) {
    tactile_timer = n.createTimer(ros::Rate(tactile_pub_freq).expectedCycleTime(), &Tactile::Pump, tact);
  }
#endif // OWDSIM

  ros::MultiThreadedSpinner s(3);
  ROS_DEBUG("Spinning");
  ros::spin(s);
  ROS_DEBUG("Done spinning; exiting");
#ifndef OWDSIM
  while (wamdriver->owam->bus) {
    usleep(10000);
  }
#endif
  return(0);
}
