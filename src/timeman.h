#ifndef TIMEMAN_H
#define TIMEMAN_H

#include "defs.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

            Time Control Variables

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// exit from engine flag
extern int quit;

// UCI "movestogo" command moves counter
extern int movestogo;

// UCI "movetime" command time counter
extern int movetime;

// UCI "time" command holder (ms)
extern int time;

// UCI "inc" command's time increment holder
extern int inc;

// UCI "starttime" command time holder
extern int starttime;

// UCI "stoptime" command time holder
extern int stoptime;

// variable to flag time control availability
extern int timeset;

// variable to flag when the time is up
extern int stopped;

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

           Misc Functions Stolen From
       Richard Allbert aka BluefeverSoftware
            (Mostly for UCI Protocol
                + time control)

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// get time (use int start = getTime() --> and then (getTime - start))
int getTime();

// leaf nodes (number of positions in test)
extern U64 nodes;
extern int counter;

int input_waiting();

// read GUI/user input
void read_input();

// a bridge function to interact between search and GUI input
static inline void communicate() {
	// if time is up break here
    if(timeset == 1 && getTime() > stoptime) {
		// tell engine to stop calculating
		stopped = 1;
	}

    // read GUI input
	read_input();
}

#endif // TIMEMAN_H
