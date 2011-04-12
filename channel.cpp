/** Distributed, single reader, typed channels */

#include "channel.h"

volatile unsigned short _channel::nextPort = 2001;

/** Add fd to set and update maxFd if necessary */
void _channel::fdSet (FD fd, fd_set* fdSet, FD* maxFd) {
	FD_SET (fd, fdSet);
	if (fd > *maxFd) *maxFd = fd;
}
