//		commands.cpp
//********************************************
#include "commands.h"
//********************************************
// function name: ExeCmd
// Description: interperts and executes built-in commands
// Parameters: pointer to jobs, command string
// Returns: 0 - success,1 - failure
//**************************************************************************************
using namespace std;

const char* QuitExcept::what() const noexcept {
  return nullptr;
}

// helper functions

bool checkInvalidArgsFgBg(vector<string>& commandVector){
	if (commandVector.size() > 2){
		return true;
	}
	if (commandVector.size() == 2){
		for (const auto& c : commandVector[1]){
			if (!isdigit(c)){
				return true;
			}
		}
	}
	return false;
}

bool checkInvalidArgsKill(vector<string>& commandVector){
	if (commandVector.size() != 3){
		return true;
	}
	else { // commandVector.size() == 3
		if (commandVector[1].at(0) != '-'){
			return true;
		}
		for (const auto& c : commandVector[1].substr(1)){
			if (!isdigit(c)){
				return true;
			}
		}
		for (const auto& c : commandVector[2]){
			if (!isdigit(c)){
				return true;
			}
		}
	}
	return false;
}

// job class implementation
Job::Job(pid_t pid, int jobId, bool isDone, bool isBg, string cmd,
		 time_t insertTime, bool isStopped) : 
		 pid(pid), jobId(jobId), isDone(isDone), isBg(isBg), cmd(cmd),
		 insertTime(insertTime), isStopped(isStopped){
}

Job::~Job(){
}

pid_t Job::getPid(){
	return pid;
}

int Job::getJobId(){
	return jobId;
}

bool Job::getIsDone(){
	return isDone;
}

bool Job::getIsBg(){
	return isBg;
}

string Job::getCmd(){
	return cmd;
}

time_t Job::getinsertTime(){
	return insertTime;
}

bool Job::getIsStopped(){
	return isStopped;
}

void Job::setPid(pid_t pid){
	this->pid = pid;
}

void Job::setJobId(int jobId){
	this->jobId = jobId;
}

void Job::setIsDone(bool isDone){
	this->isDone = isDone;
}

void Job::setIsBg(bool isBg){
	this->isBg = isBg;
}

void Job::setCmd(string cmd){
	this->cmd = cmd;
}

void Job::setinsertTime(time_t insertTime){
	this->insertTime = insertTime;
}

void Job::setIsStopped(bool isStopped){
	this->isStopped = isStopped;
}

unsigned int Job::getTimeElapsed(){
	time_t currentTime = time(0);
	return static_cast<unsigned int>(difftime(currentTime, insertTime));
}

//********************************************************************************
// smasher class implementation
Smasher::Smasher() : currPwd(string()), oldPwd(string()), runningJobs(0),
					 maxJobId(0), jobs(map<int, Job>()), fgPid(-1), fgCmd(string()), fgJobId(-1){
}

Smasher::~Smasher(){
	// delete &jobs;
}

// call this function in the main function
int Smasher::initSmash(){
	oldPwd = string();

	char buffer[MAX_LINE_SIZE];
	if (getcwd(buffer, MAX_LINE_SIZE) == nullptr){ return 1; }
	currPwd = string(buffer);

	return 0;
}

string Smasher::getCurrPwd(){
	return this->currPwd;
}

string Smasher::getOldPwd(){
	return this->oldPwd;
}

pid_t Smasher::getFgPid(){
	return this->fgPid;
}

string Smasher::getFgCmd(){
	return this->fgCmd;
}

pid_t Smasher::getFgJobId(){
	return this->fgJobId;
}

void Smasher::setFgPid(pid_t pId){
	this->fgPid = pId;
}

void Smasher::setFgCmd(string cmdString){
	this->fgCmd = cmdString;
}

void Smasher::setFgJobId(int jobId){
	this->fgJobId = jobId;
}

/**
 * @brief goes through the jobs map and cleans up any finished jobs
 * 		  needs to be called before any command is executed
 * 		  uses waitpid with WNOHANG to check if a job is done
*/
int Smasher::checkJobs(){
	int status;
	for (auto it = jobs.begin(); it != jobs.end();){
		int waitpidResult = waitpid(it->second.getPid(), &status, WUNTRACED | WNOHANG);
		if (waitpidResult > 0){ // job is done
			if (WIFEXITED(status) || WIFSIGNALED(status)){ // got exited or killed
				// cout << "cleaned by check jobs: " << it->second.getJobId() << endl;
				it = jobs.erase(it); // remove job from map
			}
			else if (WIFSTOPPED(status)){ // got stopped // TODO: check if this is the right way to check if stopped
				it->second.setIsStopped(true);
				++it;
			}
		}
		else if (waitpidResult == -1){ // waitpid failed
			perror("smash error: waitpid failed");
			return 1;
		}
		else if (waitpidResult == 0){ // job is still running
			++it;
		}
		else{
			++it;
		}
	}
	// update runningJobs
	runningJobs = jobs.size();
	// update maxJobId
	if (jobs.size() == 0){
		maxJobId = 0;
	}
	else{
		maxJobId = jobs.rbegin()->first;
	}
	return 0;
}


int Smasher::execute(string cmdString){
	//clean command and save as vector<string>
	string delimiters = " \t\n"; 
	vector<string> commandVector;
	size_t start = 0, end = 0;
	// split command string into vector of arguments
	while ((end = cmdString.find_first_of(delimiters, start)) != std::string::npos) {
		if (end != start) {
			commandVector.push_back(cmdString.substr(start, end - start));
		}
		start = end + 1;
	}
	if (start < cmdString.length()) {
		commandVector.push_back(cmdString.substr(start));
	}

	// clean up finished jobs
	checkJobs(); // maybe in other places as well
	// check if command is a background command
	if (!BgCmd(commandVector, cmdString)){ 
		return 0;
	}
	// execute command
	ExeCmd(commandVector, cmdString);
	return 0;
}

int Smasher::erasejob(int jobId){
	// Job& job = this->jobs[jobId];
	this->jobs.erase(jobId);
	if (this->jobs.size() <= 0){
		this->maxJobId = 0;
	}
	else{
		this->maxJobId = this->jobs.rbegin()->first;
	}
	// delete(job);

	return 0;
}

int Smasher::addJob(pid_t pid, int jobId, bool isBg, string cmd, bool isStopped){
	if ((jobId == -1)){
		jobId = ++this->maxJobId;
	}
	Job job = Job(pid, jobId, false, isBg, cmd, time(0), isStopped);
	jobs.insert({job.getJobId(), job});
	return 0;
}


int Smasher::ExeCmd(vector<string>& commandVector, string& cmdString)
{
	// bool illegal_cmd = false; // illegal command
	string cmd; 
	cmd = commandVector[0];

	/*************************************************/
	if (cmd.compare("showpid") == 0) // showpid
	{
		pid_t pid = getpid(); // get process id, always successful
		cout << "smash pid is " << pid << endl;
	}
	/*************************************************/
	else if (cmd.compare("pwd") == 0) // pwd
	{

		char buffer[MAX_LINE_SIZE];
		if (getcwd(buffer, MAX_LINE_SIZE) != nullptr){
			cout << buffer << endl;
		}
		else{
        	perror("smash error: getcwd failed");
    	}
	}
	/*************************************************/
	else if (cmd.compare("cd") == 0) // cd <path> 
	{
		if (commandVector.size() > 2){ //check for too many args
			cerr << "smash error: cd: too many arguments" << endl;
		}
		else if (commandVector.size() == 1){ // cd was called
			return 0;
		}
		else if (commandVector[1].compare("-") == 0){ // cd -  was called
			if (this->oldPwd.size()){
				if (!(chdir(oldPwd.c_str()) == -1)){
					this->currPwd.swap(this->oldPwd);
				}
				else{
					perror("smash error: chdir failed");
				}
			}
			else{
				cerr << "smash error: cd: OLDPWD not set" << endl;
			}
		}
		else{ // commandVector[1] is an address
			if(!(chdir(commandVector[1].c_str()) == -1)){
				this->oldPwd = this->currPwd;
				this->currPwd = commandVector[1];
			}
			else{
				perror("smash error: chdir failed");
			}
		}

	} 
	/*************************************************/
	else if (cmd.compare("jobs") == 0) // jobs
	{
		for (auto& pair : jobs){
			cout << "[" << pair.second.getJobId() << "] " <<
			pair.second.getCmd() << " : " << pair.second.getPid() << " " <<
			pair.second.getTimeElapsed() << " secs";
			
			if (pair.second.getIsStopped()){
				cout << " (stopped)";
			}
			cout << endl;
		}
	}
	/*************************************************/
	else if (cmd.compare("kill") == 0) // kill -<signum> <job-id>
	{
		// check for invalid arguments
		if (checkInvalidArgsKill(commandVector)){
			cerr << "smash error: kill: invalid arguments" << endl;
			return 1;
		}
		int signum = atoi(commandVector[1].substr(1).c_str());
		int jobId = atoi(commandVector[2].c_str());
		// check if job exists in jobs
		if (jobs.find(jobId) == jobs.end()){
			cerr << "smash error: kill: job-id " << jobId << " does not exist" << endl;
			return 1;
		}
		// send signal to job
		int result = kill(jobs[jobId].getPid(), signum);
		if (result == 0){
			cout << "signal number " << signum << " was sent to pid " << jobs[jobId].getPid() << endl;
			return 0;
		}
		else{
			perror("smash error: kill failed");
			return 1;
		}
	} 
	/*************************************************/
	else if (cmd.compare("fg") == 0) // fg <job-id>
	{	
		// if no job id was given, continue the last job in the foreground
		if (commandVector.size() == 1){ // only fg
			if (jobs.size() == 0){
				cerr << "smash error: fg: jobs list is empty" << endl;
				return 1;
			}

			if (this->jobs.find(this->maxJobId) == this->jobs.end()) {
				return 1;
			}
			
			pid_t jobPid = (this->jobs[this->maxJobId]).getPid();
			int jobId = (this->jobs[this->maxJobId]).getJobId();
			string jobCmd = (this->jobs[this->maxJobId]).getCmd();

			cout << jobCmd << " : " << jobPid << endl;
			if (kill(jobPid, SIGCONT) == -1){ // starts process if stopped
				perror("smash error: kill failed");
				return 1;
			}
			
			this->erasejob(this->maxJobId); // remove from job list
			this->fgPid = jobPid;
			this->fgCmd = jobCmd;
			this->fgJobId = jobId;
			int status;
			if (waitpid(jobPid, &status, WUNTRACED) == -1) {
				if (errno == EINTR) {// Waitpid was interrupted by a signal
					return 0;
				} else {
					perror("smash error: waitpid failed");
					return 1;
            }
        }
			// if (WIFSTOPPED(status)){ // keyboard action
			// 	this->addJob(jobPid, jobId, false, jobCmd, true);
			// }
			if (WIFEXITED(status) || WIFSIGNALED(status)){ // process ended
				this->fgPid = -1;
				this->fgCmd = "";
				this->fgJobId = -1;
			}
			return 0;
		}
		// check for invalid arguments
		if (checkInvalidArgsFgBg(commandVector)){
			cerr << "smash error: fg: invalid arguments" << endl;
			return 1;
		}
		int jobId = atoi(commandVector[1].c_str());
		// check if job exists in jobs
		if (jobs.find(jobId) == jobs.end()){
			cerr << "smash error: fg: job-id " << commandVector[1] << " does not exist" << endl;
			return 1;
		}

		pid_t jobPid = this->jobs[jobId].getPid();
		string jobCmd = this->jobs[jobId].getCmd();
		cout << jobCmd << " : " << jobPid << endl;
		if (kill(jobPid, SIGCONT) == -1){
			perror("smash error: kill failed");
			return 1;
		}
		this->erasejob(jobId); // remove from job list

		this->fgPid = jobPid;
		this->fgCmd = jobCmd;
		this->fgJobId = jobId;
		int status;
		if (waitpid(jobPid, &status, WUNTRACED) == -1) {
            if (errno == EINTR) { // Waitpid was interrupted by a signal
                return 0;
            } else {
                perror("smash error: waitpid failed");
                return 1;
            }
        }
		// if (WIFSTOPPED(status)){ // keyboard action
		// 	this->addJob(jobPid, fgJobId, false, jobCmd, true);
		// }
		else if (WIFEXITED(status) || WIFSIGNALED(status)){ // process ended
			this->fgPid = -1;
			this->fgCmd = "";
			this->fgJobId = -1;
		}
		return 0;
	} 
	/*************************************************/
	else if (cmd.compare("bg") == 0) // bg <job-id>
	{
  		// if no job id was given, continue the stopped job with the highest job id
		if (commandVector.size() == 1){
			for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) { // iterate through jobs in reverse order // might be wrong
				auto& pair = *it;
				if (pair.second.getIsStopped()){
					cout << pair.second.getCmd() << " : " << pair.second.getPid() << endl;
					if(kill(pair.second.getPid(), SIGCONT) == -1){
						perror("smash error: kill failed");
						return 1;
					}
					pair.second.setIsBg(true);
					pair.second.setIsStopped(false);
					return 0;
				}
			}
			cerr << "smash error: bg: there are no stopped jobs to resume" << endl;
			return 1;
		}
		// check for invalid arguments
		if (checkInvalidArgsFgBg(commandVector)){
			cerr << "smash error: bg: invalid arguments" << endl;
			return 1;
		}
		int jobId = atoi(commandVector[1].c_str());
		// check if job exists in jobs
		if (jobs.find(jobId) == jobs.end()){ // job does not exist
			cerr << "smash error: bg: job-id " << commandVector[1] << " does not exist" << endl;
			return 1;
		}
		// if job is stopped, send SIGCONT
		if (jobs[jobId].getIsStopped()){
			cout << jobs[jobId].getCmd() << " : " << jobs[jobId].getPid() << endl;
			if (kill(jobs[jobId].getPid(), SIGCONT) == -1){
				perror("smash error: kill failed");
				return 1;
			}
			jobs[jobId].setIsStopped(false);
			jobs[jobId].setIsBg(true);
			return 0;
		}
		else{
			cerr << "smash error: bg: job-id " << commandVector[1] 
				 << " is already running in the background" << endl;
			return 1;
		}
		return 0;
	}
	else if (cmd.compare("quit") == 0) // quit [kill]
	{
		if ((commandVector.size() > 1 && commandVector[1].compare("kill") == 0)){
			// Iterate using while loop and manually control the iterator
			auto it = jobs.begin();
			while (it != jobs.end()) {
				pid_t jobPid = it->second.getPid();
				int jobId = it->second.getJobId();

				cout << "[" << jobId << "] " << it->second.getCmd() << " - Sending SIGTERM... ";
				if (kill(jobPid, SIGTERM) == -1){
					perror("smash error: kill failed");
				} else {
					// Wait for 5 seconds to allow graceful termination
					sleep(5);
					int status;
					int waitpidResult = waitpid(jobPid, &status, WNOHANG);
					if (waitpidResult == 0){ // job is still running
						cout << "(5 sec passed) Sending SIGKILL... ";
						if (kill(jobPid, SIGKILL) == -1){
							perror("smash error: kill failed");
						}
					} else if (waitpidResult == -1){ // waitpid failed
						perror("smash error: waitpid failed");
					}
					cout << "Done." << endl;
				}
				// Erase job and safely move to the next item
				it = jobs.erase(it);
			}
		}
		throw QuitExcept();  
	}
	/*************************************************/	
	else if (cmd.compare("diff") == 0) // diff <file1> <file2>
	{
		if (commandVector.size() != 3){
			cerr << "smash error: diff: invalid arguments" << endl;
			return 1;
		}
		/***********************************************************************/
		// check if files exist
		if (access(commandVector[1].c_str(), F_OK) == -1){
			perror("smash error: access failed");
			return 1;
		}
		if (access(commandVector[2].c_str(), F_OK) == -1){
			perror("smash error: access failed");
			return 1;
		}
		// // check if files are regular files
		// struct stat file1, file2;
		// if (stat(commandVector[1].c_str(), &file1) == -1){
		// 	perror("smash error: diff: stat failed");
		// 	return 1;
		// }
		// if (stat(commandVector[2].c_str(), &file2) == -1){
		// 	perror("smash error: diff: stat failed");
		// 	return 1;
		// }
		// if (!S_ISREG(file1.st_mode)){
		// 	cerr << "smash error: diff: file1 is not a regular file" << endl;
		// 	return 1;
		// }
		// if (!S_ISREG(file2.st_mode)){
		// 	cerr << "smash error: diff: file2 is not a regular file" << endl;
		// 	return 1;
		// }
		/***********************************************************************/
		// compare files, return 1 if different, 0 if same
		ifstream input1(commandVector[1]), input2(commandVector[2]);
		if (!input1.is_open() || !input2.is_open()){
			perror("smash error: ifstream is_open failed");
			return 1;
		}
		istreambuf_iterator<char> begin1(input1), end1;
		istreambuf_iterator<char> begin2(input2), end2;
		while (begin1 != end1 && begin2 != end2){
			if (*begin1 != *begin2){
				cout << 1 << endl;
				return 0;
			}
			++begin1;
			++begin2;
		}
		if (begin1 != end1 || begin2 != end2){ // files are different lengths
			cout << 1 << endl;


			return 0;
		}
		cout << 0 << endl;
		return 0;
	}
	
	/*************************************************/	
	else // external command
	{
		int status;
 		int pID = ExeExternal(commandVector);
		if (pID == -1){
			return 1;
		}
		// wait for child to finish
		this->fgPid = pID;
		this->fgCmd = cmdString;
		this->fgJobId = -1;
		// if (waitpid(pID, &status, WUNTRACED) == -1){
		// 	perror("smash error: waitpid failed");
		// 	return 1;
		// }
		if (waitpid(pID, &status, 0) == -1) {
            if (errno == EINTR) { // Waitpid was interrupted by a signal
                return 0; 
            } else {
                perror("smash error: waitpid failed");
                return 1;
            }
        }
		// check if child was stopped
		// if (WIFSTOPPED(status)){
		// 	Job job = Job(pID, maxJobId++, false, false, cmdString, time(0), true);
		// 	jobs.insert({job.getJobId(), job});
		// }
		if (WIFEXITED(status) || WIFSIGNALED(status)){ // process ended or killed
			// int exit_status = WEXITSTATUS(status);
			// printf("Exit status of the child was %d\n", exit_status);
			this->fgPid = -1;
			this->fgCmd = "";
		}
	 	return 0;
		
	}

	// if (illegal_cmd == true)
	// {
	// 	cerr << "smash error: > \"" << cmdString << "\"" << endl;
	// 	return 1;
	// }
    return 0;
}




//**************************************************************************************
// function name: ExeExternal
// Description: executes external command
// Parameters: external command arguments, external command string
// Returns: int
//**************************************************************************************
int Smasher::ExeExternal(vector<string>& args)
{
	string cmd = args[0]; // args[1]+ are arguments, while cmd is a path to executable
	int pID = fork();
	std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(&arg.front());
    }
    argv.push_back(nullptr);

    switch(pID) 
	{
    		case -1: // error
					perror("smash error: fork failed");
					return -1; // need to return 1?
        	case 0 : // Child Process
               		setpgrp(); // set child process group id to its own pid
					execv(cmd.c_str(), argv.data());
					// if execv returns, it failed
					perror("smash error: execv failed");
					exit(1);
			default: // Parent Process
				return pID;
	}
}

//**************************************************************************************
// function name: BgCmd
// Description: if command is in background, insert the command to jobs
// Parameters: command string, pointer to jobs
// Returns: 0- BG command -1- if not
//**************************************************************************************
// builtin no &  -> -1
// builtin yes & -> -1 (we cleaned the &)
// external no & -> -1
// extrenal yes & -> 0
int Smasher::BgCmd(vector<string>& commandVector, string& cmdString){
	if (commandVector[commandVector.size()-1] != "&"){
		return -1;
	}
	commandVector.pop_back(); // remove the & from the command string
	for (auto& e : builtInCommands){
		if (commandVector[0] == e){
			return -1;
		}
	}
	// background command code :
	int pID = ExeExternal(commandVector);
	if (pID == -1){
		return -1;
	}
	Job job = Job(pID, ++maxJobId, false, true, cmdString, time(0), false);
	jobs.insert({job.getJobId(), job});
	
	return 0;
}