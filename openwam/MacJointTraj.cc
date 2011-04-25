/***********************************************************************

  Copyright 2007-2011 Carnegie Mellon University and Intel Corporation
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

#include "MacJointTraj.hh"
#include <syslog.h>
#include "MacQuinticBlend.hh"

#define VERBOSE 0

namespace OWD {

MacJointTraj::~MacJointTraj() {
  for (unsigned int i=0; i<macpieces.size(); ++i) {
    delete macpieces[i];
  }
}


MacJointTraj::MacJointTraj(TrajType &vtraj, 
			   const JointPos &mjv, 
			   const JointPos &mja,
			   double max_jerk,
			   bool bWaitForStart,
			   bool bHoldOnStall,
			   bool bHoldOnForceInput) :
  OWD::Trajectory("MacJointTraj"), max_joint_vel(mjv), max_joint_accel(mja)
{
  // initialize the base class members
  id = 0;
  WaitForStart=bWaitForStart;
  HoldOnStall=bHoldOnStall;
  HoldOnForceInput=bHoldOnForceInput;

  // unlike previous trajectory code, this package assumes that
  // extraneous co-linear points have already been removed, so that
  // every remaining point is a bend or inflection.

    //    pthread_mutex_init(&mutex, NULL);
    if (vtraj.size() < 2) {
        throw "Trajectories must have 2 or more points";
    }

    DOF = vtraj[0].size();

    int numpoints = vtraj.size();
    start_position = vtraj.front();
    
    MacQuinticSegment *openseg; // this will hold the seg we're working on

    // make sure the trajectory specifies no blends at the ends
    vtraj[0].blend_radius = 0;
    vtraj.back().blend_radius = 0;

    if (VERBOSE) {
      printf("======== Segment 1 ===============\n");
    }
    if (numpoints == 2) {
      // if there are only 2 points, we'll just make a single segment
      // with no blends at either end
      openseg = new MacQuinticSegment(vtraj[0],vtraj[1],
				      max_joint_vel,
				      max_joint_accel,
				      max_jerk);
      openseg->setVelocity(0,0);

    } else {

      // initialize the first segment with zero starting velocity
      openseg = new MacQuinticSegment(vtraj[0],vtraj[1],
				      max_joint_vel,
				      max_joint_accel,
				      max_jerk);
      openseg->setVelocity(0,MacQuinticElement::VEL_MAX);

    }

    // now, go through the points and build the remaining segments
    // and blends

    for (int i = 2; i < numpoints; ++i) {        

      if (VERBOSE) {
	printf("======== Segment %d ==============\n",i);
      }
      // make the next segment
      MacQuinticSegment *nextseg;
      if (i==numpoints-1) {
	// the last segment doesn't get a final blend
	nextseg = new MacQuinticSegment(vtraj[i-1],vtraj[i],
					max_joint_vel,
					max_joint_accel,
					max_jerk);
	// set to come to a stop
	nextseg->setVelocity(openseg->EndVelocity(),0);
      } else {
	// all other segments expect blends at both ends (default)
	nextseg = new MacQuinticSegment(vtraj[i-1],vtraj[i],
					max_joint_vel,
					max_joint_accel,
					max_jerk);
	// set to increasing velocity (if possible)
	nextseg->setVelocity(openseg->EndVelocity(),
			     MacQuinticElement::VEL_MAX);
      }

      // add the current segment to the trajectory
      macpieces.push_back(openseg);

      if (vtraj[i-1].blend_radius > 0) {
	if (VERBOSE) {
	  printf("======== Blend %d:%d ======================\n",i-1,i);
	}
	// make the blend between the current and next
	MacQuinticBlend *blend = new MacQuinticBlend(openseg, nextseg,
						     vtraj[i-1].blend_radius,
						     max_joint_vel,
						     max_joint_accel,
						     max_jerk);

	// calculate the max blend velocity, based on previous segment
	// and dynamic limits
	blend->setStartVelocity(openseg->EndVelocity());
	
	// propogate the blend velocity to the two matching segment ends
	// (this may reduce the ending velocity of the openseg)
	if (VERBOSE) {
	  printf("======== Blend seg %d ======================\n",i-1);
	}
	openseg->setEndVelocity(blend->StartVelocity());
	if (VERBOSE) {
	  printf("======== Blend seg %d ======================\n",i);
	}
	nextseg->setStartVelocity(blend->EndVelocity());
      
	// add the blend to the trajectory
	macpieces.push_back(blend);
      } else {
	// must come to a stop before abruptly changing direction
	if (VERBOSE) {
	  printf("======== Blend seg %d ======================\n",i-1);
	}
	openseg->setEndVelocity(0);
	if (VERBOSE) {
	  printf("======== Blend seg %d ======================\n",i);
	}
	nextseg->setStartVelocity(0);
      }

      // step forward
      openseg = nextseg;

    }

    // add the final segment
    macpieces.push_back(openseg);

    // now we have a collection of segments and blends in which we always
    // tried to accelerate as much as possible, from beginning to end.  It's
    // time to go backwards from the end to make sure we can decelerate
    // in time for the final condition (v=0) and each of the blend limits.
    // We'll do this by using the base class interface so that we don't have 
    // to care if we're looking at a segment or a blend.

    for (unsigned int i = macpieces.size() -1; i>0; --i) {
      if (VERBOSE) {
	printf("==== Backwards pass, piece %d ====\n",i);
      }
      if (macpieces[i-1]->EndVelocity() > macpieces[i]->StartVelocity()) {
	if (VERBOSE) {
	  printf("WARNING: reducing speed of macpiece %d from %2.3f to %2.3f\n",
		 i-1,macpieces[i-1]->EndVelocity(),macpieces[i]->StartVelocity());
	}
	macpieces[i-1]->setEndVelocity(macpieces[i]->StartVelocity());
      }
      if (macpieces[i-1]->EndVelocity() < macpieces[i]->StartVelocity()) {
	free(macpieces[i]->reason);
	macpieces[i]->reason = strdup("limited by previous segment ending velocity");
	macpieces[i]->setStartVelocity(macpieces[i-1]->EndVelocity());
      }
    }

    // finally, calculate all the curve coefficients
    for (unsigned int i=0; i<macpieces.size(); ++i) {
      if (VERBOSE) {
	printf("==== BuildProfile, piece %d ====\n",i);
      }
      macpieces[i]->BuildProfile();
      if (i<macpieces.size()-1) {
	// propogate the time forward to the next element
	macpieces[i+1]->setStartTime(macpieces[i]->EndTime());
      }
    }

    traj_duration = macpieces.back()->EndTime();
    end_position = vtraj.back();

    current_piece =macpieces.begin();
    return;
}

void MacJointTraj::get_path_values(double *path_vel, double *path_accel) const {
  *path_vel = (*current_piece)->PathVelocity();
  *path_accel = (*current_piece)->PathAcceleration();
}

void MacJointTraj::get_limits(double *max_path_vel, double *max_path_accel) const {
  *max_path_vel = (*current_piece)->MaxPathVelocity();
  *max_path_accel = (*current_piece)->MaxPathAcceleration();
}

void MacJointTraj::evaluate(OWD::Trajectory::TrajControl &tc, double dt) {

  if (tc.q.size() < DOF) {
    runstate = DONE;
    return;
  }

  // if we're running, then increment the time.  Otherwise, stay where we are
  if ((runstate == RUN) || (runstate == LOG)) {
    time += dt;
  }
  
  while ((current_piece != macpieces.end()) &&
	 (time > (*current_piece)->EndTime())) {
           
    // keep skipping forward until we find the one that includes this time
    ++current_piece;
  }
  if (current_piece == macpieces.end()) {
    // even though time is past the end, the piece will return the ending
    // values
    current_piece--;
    (*current_piece)->evaluate(tc,time);
    runstate = DONE;
  } else {
    (*current_piece)->evaluate(tc,time);
  }
  if (runstate == STOP) {
    // if we're supposed to be stationary, keep the position values we
    // calculated, but zero out the vel and accel
    for (int j=0; j < DOF; ++j) {
      tc.qd[j]=tc.qdd[j]=0.0;
    }
  }
  return;
}

void MacJointTraj::run() {
    if (runstate==DONE || runstate==RUN) {
        return;
    }
    if (time > 0.0f) {
      syslog(LOG_ERR,"ERROR: Attempted to restart a trajectory in the middle");
      return;
    }
    runstate=RUN;
    return;
}

void MacJointTraj::log(char *trajname) {
    if ((runstate != STOP) && (runstate != DONE)) {
      throw "can't log a running trajectory";
    }
    double oldtime=time;
    current_piece = macpieces.begin();
    char *simfname = (char *) malloc(strlen(trajname) +9);
    // start with the logfilename
    sprintf(simfname,"%s_sim.csv",trajname);
    FILE *csv = fopen(simfname,"w");
    if (csv) {
      OWD::Trajectory::TrajControl tc(DOF);
        runstate=LOG;
        time=0.0;
        while (runstate == LOG) {
            evaluate(tc,0.01);
            fprintf(csv,"%3.8f, ",time);
            for (int j=0; j<DOF; ++j) {
                fprintf(csv,"%2.8f, ",tc.q[j]);
            }
            for (int j=0; j<DOF; ++j) {
                fprintf(csv,"%2.8f, ",tc.qd[j]);
            }
            for (int j=0; j<DOF; ++j) {
                fprintf(csv,"%2.8f, ",tc.qdd[j]);
            }
            for (int j=0; j<DOF; ++j) {
                fprintf(csv,"%2.8f, ",tc.t[j]);
            }
        }
        fclose(csv);
    }
    reset(oldtime);
    runstate=STOP;
    free(simfname);
    return;
}

void MacJointTraj::dump() {
  printf("MacJointTraj: %zd pieces, %2.3fs total duration\n",
	 macpieces.size(), traj_duration);
  for (unsigned int i=0; i<macpieces.size(); ++i) {
    macpieces[i]->dump();
  }
}

void MacJointTraj::reset(double t) {
  time=t;
  current_piece=macpieces.begin();
}

}; // namespace OWD
