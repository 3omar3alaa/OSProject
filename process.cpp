#include <cstdlib>
using namespace std;
#include "headers.h"

//Variables
long remainingtime;
long lastRun = -1;
sigset_t sigSet;
struct sigaction siga;

//Functions
void workAround(int) {
	raise(SIGURG);
}
void recordStart(int) {
	lastRun = getClk();
	cout << "YYYYYYYYYYYYYYYYYYYYYYYYYYYProc " << getpid() << ": lastRun now is " << lastRun << endl;
}
void recordEnd(int) {
	remainingtime -= getClk() - lastRun;
	cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXProc " << getpid() << ": remTime now is " << remainingtime << endl;
	raise(SIGSTOP);
}

int main(int agrc, char* argv[]) {
	initClk();
	lastRun = getClk();
	cout << "Proc "<<getpid()<<": Starting with running time of " << argv[0] << endl;
	remainingtime = strtol(argv[0], NULL, 10); //TODO: No error handling

	//remove blocking of signals done by parent
	sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGCHLD);
	sigaddset(&sigSet, SIGCONT);
	sigaddset(&sigSet, SIGURG);
	sigprocmask(SIG_UNBLOCK, &sigSet, NULL);	

	//handle SIGCONT signal
	signal(SIGCONT, recordStart);
	signal(SIGTSTP, recordEnd);

	//handle SIGTSTP and make it uninterruptable
	/*sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGURG);
	siga.sa_handler = recordEnd;
	siga.sa_mask = sigSet;
	siga.sa_flags = 0;
	sigaction(SIGTSTP, &siga, NULL);*/

	while (remainingtime > (getClk() - lastRun)) {
		sleep(2);
		cout << "Proc " << getpid() << ": TICK TICK" << endl;
	}
	cout << "Proc " << getpid() << ": finished! Returning ..." << endl;
	destroyClk(false);
	return 0;
}
