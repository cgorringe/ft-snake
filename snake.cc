// snake (client)
// Copyright (c) 2019 Carl Gorringe (carl.gorringe.org)
// 3/9/2019
//
// Client Snake Game for the Flaschen Taschen LED Display.
// See the README.md for more info.
//
// --------------------------------------------------------------------------------
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>
//
// --------------------------------------------------------------------------------

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

// global vars
double opt_timeout = 20; // timeout in 30 secs (TEST)
int opt_delay = 50;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

int main(int argc, char *argv[]) {

  printf("ft-snake (c) 2019 Carl Gorringe (carl.gorringe.org)\n\n");

  // handle break
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  // TODO: Check that server is running...

  printf("Playing Snake!\nUse WASD keys to move.\n");

  // TEST output
  printf("You are [color] #000000\n");

  // run loop
  time_t starttime = time(NULL);
  double dtime = 0;
  int mins = 0, secs = 0, score = 0;
  do {
    dtime = difftime(time(NULL), starttime);
    mins = dtime / 60;
    secs = (int)dtime % 60;
    printf("\r%02d:%02d (score: %d) ", mins, secs, score);
    fflush(stdout);

    usleep(opt_delay * 1000);

  } while ( (dtime <= opt_timeout) && !interrupt_received );

  printf("\rYou Died at %02d:%02d (score: %d)      \n\n", mins, secs, score);

  if (interrupt_received) return 1;
  return 0;
}