#pragma once

#include "signal.h"
#include "stdio.h"

#define ASSERT(x)  do { if (!(x)) { fprintf(stderr, "%s:%d: Assertion '" #x "' failed", __FILE__, __LINE__); raise(SIGINT); }}  while(0)
