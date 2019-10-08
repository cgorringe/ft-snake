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

#include "udp-flaschen-taschen.h"
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <signal.h>

// Defaults
#define DISPLAY_WIDTH  (9*5)
#define DISPLAY_HEIGHT (8*5)  // use 7*5 for original flaschen-taschen
#define Z_LAYER 3      // (0-15) 0=background
#define DELAY 50      // 150
#define MAX_SNAKE_LEN 1000
#define MAX_PLAYERS 100

#define TYPE_CLIENT 1
#define TYPE_SERVER 2
#define TYPE_LOCAL  3
#define TRUE 1
#define FALSE 0

typedef struct SnakePos {
  short x;
  short y;
} SnakePos_t;

typedef struct Snake {
  int id;
  short head_x;
  short head_y;
  int length;
} Snake_t;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

// ------------------------------------------------------------------------------------------
// Command Line Options

// option vars
const char *opt_hostname = NULL;
int opt_layer  = Z_LAYER;
int opt_width  = DISPLAY_WIDTH;
int opt_height = DISPLAY_HEIGHT;
int opt_xoff=0, opt_yoff=0;
int opt_delay  = DELAY;
double opt_timeout = 10;  // TEST
int opt_type = 0;         // 0=none, 1=server, 2=client, 3=both

int usage(const char *progname) {

  fprintf(stderr, "ft-snake (c) 2019 Carl Gorringe (carl.gorringe.org)\n");
  fprintf(stderr, "Usage: %s [options] {local|client|server} \n", progname);
  fprintf(stderr, "Options:\n"
    "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
    "\t-l <layer>     : Layer 0-15. (default 10)\n"
    "\t-t <timeout>   : Timeout exits after given seconds. (default ?)\n"
    "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
    "\t-d <delay>     : Delay between frames in milliseconds. (default 50)\n"
  );
  return 1;
}

int cmdLine(int argc, char *argv[]) {

  // command line options
  int opt;
  while ((opt = getopt(argc, argv, "?g:l:t:h:d:")) != -1) {
    switch (opt) {
    case '?':  // help
      return usage(argv[0]);
      break;
    case 'g':  // geometry
      if (sscanf(optarg, "%dx%d%d%d", &opt_width, &opt_height, &opt_xoff, &opt_yoff) < 2) {
        fprintf(stderr, "Invalid size '%s'\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 'l':  // layer
      if (sscanf(optarg, "%d", &opt_layer) != 1 || opt_layer < 0 || opt_layer >= 16) {
        fprintf(stderr, "Invalid layer '%s'\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 't':  // timeout
      if (sscanf(optarg, "%lf", &opt_timeout) != 1 || opt_timeout < 0) {
        fprintf(stderr, "Invalid timeout '%s'\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 'h':  // hostname
      opt_hostname = strdup(optarg); // leaking. Ignore.
      break;
    case 'd':  // delay
      if (sscanf(optarg, "%d", &opt_delay) != 1 || opt_delay < 1) {
        fprintf(stderr, "Invalid delay '%s'\n", optarg);
        return usage(argv[0]);
      }
      break;
    default:
      return usage(argv[0]);
    }
  }

  // retrieve arg text
  const char *text = argv[optind];
  if (text && strncmp(text, "server", 6) == 0) {
    opt_type = TYPE_SERVER;
  }
  else if (text && strncmp(text, "client", 6) == 0) {
    opt_type = TYPE_CLIENT;
  }
  else if (text && strncmp(text, "local", 5) == 0) {
    opt_type = TYPE_LOCAL; // TYPE_SERVER + TYPE_CLIENT
  }
  else {
    fprintf(stderr, "Missing 'local', 'client', or 'server'\n");
    return usage(argv[0]);
  }

  return 0;
}

// ------------------------------------------------------------------------------------------

// random int in range min to max inclusive
int randomInt(int min, int max) {
  return (random() % (max - min + 1) + min);
}

// ------------------------------------------------------------------------------------------

/* NOTES

Client sends chars: (on each keypress, or exit)
key \n...
  key = {'0'=exit, 'W'=up, 'A'=left, 'S'=down, 'D'=right}

Server sends 2 numbers: (on each score increase, or death)
id score \n...
  id = client id (1-254) which is also the color index
  score = 0-100 or -1 for end game

*/


// ------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

  // main vars
  int socket;

  // seed the random generator
  srandom(time(NULL));

  // parse command line
  if (int e = cmdLine(argc, argv)) { return e; }
  switch (opt_type) {
    case TYPE_CLIENT:
      fprintf(stderr, "Running Client...\n");
      break;
    case TYPE_SERVER:
      fprintf(stderr, "Running Server...\n");
      break;
    case TYPE_LOCAL:
      fprintf(stderr, "Running Locally...\n");
      break;
    default:
      fprintf(stderr, "Not Running.\n");  // should never get here
      return 1;
  }

  // handle break
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  // open socket and create our canvas
  if (opt_type & TYPE_SERVER) {
    socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.Clear();
  }

  // TODO: Check that server is running...

  // *** REDO following ***

  printf("Playing Snake!\nUse WASD keys to move.\n");

  // TEST output
  printf("You are [color] #000000\n");

  // run loop
  time_t starttime = time(NULL);
  double dtime = 0;
  int mins = 0, secs = 0, score = 0;
  do {
    // timer
    dtime = difftime(time(NULL), starttime);
    mins = dtime / 60;
    secs = (int)dtime % 60;
    printf("\r%02d:%02d (score: %d) ", mins, secs, score);

    // user input (TODO)


    fflush(stdout);
    usleep(opt_delay * 1000);

  } while ( (dtime <= opt_timeout) && !interrupt_received );

  printf("\rYou Died at %02d:%02d (score: %d)      \n\n", mins, secs, score);

  if (interrupt_received) return 1;
  return 0;
}