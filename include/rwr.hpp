#ifndef INCLUDE_RWR_HPP_
#define INCLUDE_RWR_HPP_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
int cread(int fd, char *buf, int n);

/**************************************************************************
* cwrite: write routine that checks for errors and exits if an error is  *
*         returned.                                                      *
**************************************************************************/
int cwrite(int fd, char *buf, int n);

/**************************************************************************
* read_n: ensures we read exactly n bytes, and puts them into "buf".     *
*         (unless EOF, of course)                                        *
**************************************************************************/
int read_n(int fd, char *buf, int n);

/**************************************************************************
* do_debug: prints debugging stuff (doh!)                                *
**************************************************************************/
void do_debug(char *msg, int debug, ...);

/**************************************************************************
* my_err: prints custom error messages on stderr.                        *
**************************************************************************/
void my_err(char *msg, ...);

void usage(char *progname);




#endif //  _INCLUDE_RWR_HPP_
