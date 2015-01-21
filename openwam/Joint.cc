/*
  Copyright 2006 Simon Leonard

  This file is part of openwam.

  openwam is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 3 of the License, or (at your
  option) any later version.

  openwam is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Joint.hh"

//const double Joint::MIN_POS[]={0.00,-2.62,-1.80,-2.60,-0.70,-4.30,-1.25,-2.50};
//const double Joint::MAX_POS[]={0.00, 2.62, 1.80, 2.60, 3.10, 1.00, 1.25, 2.50};

// These values are based on the motor limits that are set in Puck.cc, then
// transformed to joint limits using the IPNM and transmission ratios for
// each motor and joint.
#if !defined HEAD
const double Joint::MAX_MECHANICAL_TORQ_EXPECTED[7] = {
  //     Jtorque = MotorMax / Motor_IPNM * Motor_revs_per_joint_rev
  82.1,  // J1 = 4860 / 2472 * 41.782
  109.5, // J2 = 4860 / 2472 * 27.836 * 2.0 (combined M2 and M3)
  65.2,  // J3 = 4860 / 2472 * 27.836 * 2.0 / 1.68 (combined M2 and M3 plus diff)
  31.2,  // J4 = 4320 / 2472 * 17.86
  12.4,  // J5 = 3900 / 6105 * 9.68 * 2.0 (combined M5 and M6)
  12.4,  // J6 = 3900 / 6105 * 9.68 * 2.0 (combined M5 and M6)
  2.2    // J7 = 3200 / 21400 * 14.962
};

/* Set in WAM::init() */
double Joint::MAX_MECHANICAL_TORQ[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// The MAX_SAFE values are the most we ever want to use for each joint
double Joint::MAX_SAFE_TORQ[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Joint torques that exceed the SAFE values but are below the MAX_CLIPPABLE
// values will be clipped to the SAFE level.
const double Joint::MAX_CLIPPABLE_TORQ[7] = {
  // double the SAFE values
  165.0, // J1
  220.0, // J2
  130.0, // J3
  62.0,  // J4
  25.0,  // J5
  25.0,  // J6
  10.0   // J7
};

#else // HEAD
const double Joint::MAX_MECHANICAL_TORQ_EXPECTED[2] = {
  0.596,  // J1 = 1741 / 14524 * 3
         // J1 = 4.5*1741 / 24154
  2.33  // J2 = 1741 / 27750 * 36
  //0.12,  // J1 = 1741 / 14524 * 3
  //0.78  // J2 = 1741 / 27750 * 36
  //[3200,1800]

};
/* Set in WAM::init() */
double Joint::MAX_MECHANICAL_TORQ[2] = {0.0, 0.0};

// The MAX_SAFE values are the most we ever want to use for each joint
double Joint::MAX_SAFE_TORQ[2] = {0.0, 0.0};

// Joint torques that exceed the SAFE values but are below the MAX_CLIPPABLE
// values will be clipped to the SAFE level.
const double Joint::MAX_CLIPPABLE_TORQ[2] = {
  // double the SAFE values
  2.5, // J1
  6.2  // J2
};
#endif // HEAD

//const double Joint::VEL[]={0.00, 0.75, 0.75, 0.75, 0.75, 5.00, 5.00,10.75};
//const double Joint::ACC[]={0.00, 0.75, 0.75, 0.75, 0.75, 4.50, 4.50, 7.50};

/*
  Copyright 2006 Simon Leonard

  This file is part of openwam.

  openwam is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 3 of the License, or (at your
  option) any later version.

  openwam is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Joint.hh"

//const double Joint::MIN_POS[]={0.00,-2.62,-1.80,-2.60,-0.70,-4.30,-1.25,-2.50};
//const double Joint::MAX_POS[]={0.00, 2.62, 1.80, 2.60, 3.10, 1.00, 1.25, 2.50};

// These values are based on the motor limits that are set in Puck.cc, then
// transformed to joint limits using the IPNM and transmission ratios for
// each motor and joint.
const double Joint::MAX_MECHANICAL_TORQ_EXPECTED[7] = {
  //     Jtorque = MotorMax / Motor_IPNM * Motor_revs_per_joint_rev
  82.1,  // J1 = 4860 / 2472 * 41.782
  109.5, // J2 = 4860 / 2472 * 27.836 * 2.0 (combined M2 and M3)
  65.2,  // J3 = 4860 / 2472 * 27.836 * 2.0 / 1.68 (combined M2 and M3 plus diff)
  31.2,  // J4 = 4320 / 2472 * 17.86
  12.4,  // J5 = 3900 / 6105 * 9.68 * 2.0 (combined M5 and M6)
  12.4,  // J6 = 3900 / 6105 * 9.68 * 2.0 (combined M5 and M6)
  2.2    // J7 = 3200 / 21400 * 14.962
};

/* Set in WAM::init() */
double Joint::MAX_MECHANICAL_TORQ[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// The MAX_SAFE values are the most we ever want to use for each joint
double Joint::MAX_SAFE_TORQ[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Joint torques that exceed the SAFE values but are below the MAX_CLIPPABLE
// values will be clipped to the SAFE level.
const double Joint::MAX_CLIPPABLE_TORQ[7] = {
  // double the SAFE values
  165.0, // J1
  220.0, // J2
  130.0, // J3
  62.0,  // J4
  25.0,  // J5
  25.0,  // J6
  10.0   // J7
};

//const double Joint::VEL[]={0.00, 0.75, 0.75, 0.75, 0.75, 5.00, 5.00,10.75};
//const double Joint::ACC[]={0.00, 0.75, 0.75, 0.75, 0.75, 4.50, 4.50, 7.50};

