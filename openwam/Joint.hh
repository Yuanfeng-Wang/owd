/*
  Copyright 2006 Simon Leonard

  This file is part of openwam.

  openman is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 3 of the License, or (at your
  option) any later version.

  openman is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <iomanip>
#include <pthread.h>
#include "globals.h"

#ifndef __JOINT_HH__
#define __JOINT_HH__

using namespace std;

class Joint{
private:
  void lock(){pthread_mutex_lock(&mutex);}
  void unlock(){pthread_mutex_unlock(&mutex);}


public:
  pthread_mutex_t mutex;

  int    ID;
  double q;       // position
  double t;       // Nm or puck torque units
  
  static const int J1 = 1;
  static const int J2 = 2;
  static const int J3 = 3;
  static const int J4 = 4;
  static const int J5 = 5;
  static const int J6 = 6;
  static const int J7 = 7;

#ifdef WRIST
  static const int Jn = 7;
#else
  static const int Jn = 4;
#endif

    //  static const double MIN_POS[];
    //  static const double MAX_POS[];
    //  static const double MIN_TRQ[];
    //  static const double MAX_TRQ[];
    //  static const double VEL[];
    //  static const double ACC[];
  
  Joint(){
    pthread_mutex_init(&mutex, NULL);
    q = 0.0; t = 0.0;
  }

  int id(){return ID;}
  double trq(){  double T;  lock();  T = t;   unlock();  return T;  }
  double pos(){  double Q;  lock();  Q = q;   unlock();  return Q;  }

  void trq(double T) {  
    lock();  
    t = T; // clip( T, Joint::MIN_TRQ[id()], Joint::MAX_TRQ[id()] ); 
    //t = 0.0;
    unlock();
  }
  void pos(double Q) {  
    lock();  q = Q;  unlock(); 
    //    if(Q <= Joint::MIN_POS[id()] || Joint::MAX_POS[id()] <= Q);
    //cerr << "Joint::pos: WARNING J" << id() << " has reached its limit.\n";
  }

};

#endif

