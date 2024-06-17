#pragma once
#ifndef SIGNALS_H
#define SIGNALS_H
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <string>
#include "commands.h"
#include <setjmp.h>

void ctrlCHandler(int sigNum);
void ctrlZHandler(int sigNum);

#endif

