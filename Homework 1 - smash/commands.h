#pragma once
#ifndef COMMANDS_H
#define COMMANDS_H
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <ctime>
#include <fstream>

#define MAX_LINE_SIZE 80
#define MAX_ARG 20
// typedef enum { FALSE , TRUE } bool; 

// int BgCmd(char* lineSize, void* jobs, pSmashInfo pSmasher);
// int ExeCmd(void* jobs, char* lineSize, char* cmdString, pSmashInfo pSmasher);
// void ExeExternal(char *args[MAX_ARG], char* cmdString);

using namespace std;
extern std::vector<std::string> builtInCommands;

class QuitExcept : public std::exception
{
    public:
        QuitExcept() = default;
        const char* what() const noexcept override;
};

class Job{
private:
    pid_t pid;
    int jobId;
    bool isDone;
    bool isBg;
    string cmd;
    time_t insertTime;
    bool isStopped;
    
public:
    Job(): pid(-1), jobId(-1), isDone(false), isBg(false),
           cmd(""), insertTime(-1), isStopped(false){};
    Job(pid_t pid, int jobId, bool isDone, bool isBg, string cmd,
		 time_t insertTime, bool isStopped);
    ~Job();
    
    pid_t getPid();
    int getJobId();
    bool getIsDone();
    bool getIsBg();
    string getCmd();
    time_t getinsertTime();
    bool getIsStopped();

    void setPid(pid_t pid);
    void setJobId(int jobId);
    void setIsDone(bool isDone);
    void setIsBg(bool isBg);
    void setCmd(string cmd);
    void setinsertTime(time_t insertTime);
    void setIsStopped(bool isStopped);

    unsigned int getTimeElapsed();
};

class Smasher{
private:
    string currPwd;
    string oldPwd;
    int runningJobs;
    int maxJobId;
    map<int, Job> jobs;
    pid_t fgPid; // -1 if nothing is running in fg
    string fgCmd; // empty if nothing is running in fg
    int fgJobId; // -1 if it wasnt a job before
    
    int checkJobs();
public:
    Smasher();
    ~Smasher();
    int initSmash();
    int execute(string cmdString);

    static Smasher& getInstance() // make SmallShell singleton
    {
      static Smasher instance; // Guaranteed to be destroyed.
      // Instantiated on first use.
      return instance;
    }

    string getCurrPwd();
    string getOldPwd();
    pid_t getFgPid();
    string getFgCmd();
    int getFgJobId();
    void setFgPid(pid_t pId);
    void setFgCmd(string cmdString);
    void setFgJobId(int jobId);
    int erasejob(int jobId);
    int addJob(pid_t pid, int jobId, bool isBg, string cmd, bool isStopped);

    int BgCmd(vector<string>& commandVector, string& cmdString);
    int ExeCmd(vector<string>& commandVector, string& cmdString);
    int ExeExternal(vector<string>& args);
};

#endif

