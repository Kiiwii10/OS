/*	smash.cpp
main file. This file contains the main function of smash
*******************************************************************/
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <signal.h>
#include <limits>

#include "commands.h"
#include "signals.h"
#define MAX_LINE_SIZE 80
#define MAXARGS 20
#define MAX_JOBS 100

std::vector<std::string> builtInCommands {
"showpid",
"pwd",
"cd",
"jobs",
"kill",
"fg",
"bg",
"quit",
"diff",
};


//**************************************************************************************
// function name: main
// Description: main function of smash. get command from user and calls command functions
//**************************************************************************************
using namespace std;
int main(int argc, char *argv[])
{
	// Init globals

	string cmdString;
	Smasher& smash = Smasher::getInstance();
	
	if (smash.initSmash()){
		cout << "smash error: initSmash failed" << endl;
		exit(-1);
	}

	// Setup signal handlers
    struct sigaction actCtrlC = {};
    actCtrlC.sa_handler = ctrlCHandler;
    sigemptyset(&actCtrlC.sa_mask);
    actCtrlC.sa_flags = 0;

    if (sigaction(SIGINT, &actCtrlC, nullptr) < 0) {
        perror("sigaction");
        return 1;
    }

    struct sigaction actCtrlZ = {};
    actCtrlZ.sa_handler = ctrlZHandler;
    sigemptyset(&actCtrlZ.sa_mask);
    actCtrlZ.sa_flags = 0;

    if (sigaction(SIGTSTP, &actCtrlZ, nullptr) < 0) {
        perror("sigaction");
        return 1;
    }
	
	while (1) {
		cout << "smash > ";
		string cmdString;

		if (!getline(std::cin, cmdString)) {
			if (std::cin.eof()) {
				// Handle EOF (Ctrl+D).
				std::cin.clear();  // Clear EOF flag.
				continue;          // Continue to the next iteration of the loop.
			} else if (std::cin.fail()) {
				// Handle other input errors, possibly due to signals.
				std::cin.clear();  // Clear error flags.
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignore the rest of the line.
				continue;          // Continue to the next iteration of the loop.
			}
		}

		if (!cmdString.empty()) {
			try {
				smash.execute(cmdString);
			}
			catch (const QuitExcept& e) {
				break;  // Exit loop if needed.
			}
		}
	}

    return 0;
}

