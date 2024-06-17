// signals.cpp
// contains signal handler funtions
// contains the function/s that set the signal handlers

/*******************************************/
/* Name: handler_cntlc
   Synopsis: handle the Control-C */
#include "signals.h"


void ctrlCHandler(int sigNum){
   Smasher& smasher = Smasher::getInstance();

   cout << "smash: caught ctrl-C" << endl;
   pid_t fgPid = smasher.getFgPid();
   if (fgPid != -1){
      if (kill(fgPid, SIGKILL) == -1){
         perror("smash error: kill failed");
         return;
      }

      smasher.setFgPid(-1);
      smasher.setFgCmd("");
      smasher.setFgJobId(-1);

      cout << "smash: process " << fgPid << " was killed" << endl;
   }
}

void ctrlZHandler(int sigNum){
   Smasher& smasher = Smasher::getInstance();
   
   cout << "smash: caught ctrl-Z" << endl;
   pid_t fgPid = smasher.getFgPid();
   int fgJobId = smasher.getFgJobId();
   if (fgPid != -1){
      if (kill(fgPid, SIGSTOP) == -1){
         perror("smash error: kill failed");
         return;
      }
      smasher.addJob(fgPid, fgJobId, false, smasher.getFgCmd(), true);

      smasher.setFgPid(-1);
      smasher.setFgCmd("");
      smasher.setFgJobId(-1);
      cout << "smash: process " << fgPid << " was stopped" << endl;
   }
}
