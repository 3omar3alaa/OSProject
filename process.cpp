#include <cstdlib>
using namespace std;
#include "headers.h"

//Variables
long remainingtime;
sigset_t sigSet;
bool pauseIt;
struct sigaction siga;

//Functions
//static void multi_handler(int sig, siginfo_t *siginfo, void *context) {
//	pid_t sender_pid = siginfo->si_pid;
//	cout << "Yo: I am " << getpid() << " and " << (int)sender_pid << " woke me up ETSARAF" << endl;
//}

int main(int agrc, char* argv[]) {
	cout << "Proc "<<getpid()<<": Starting with running time of " << argv[0] << endl;
	remainingtime = strtol(argv[0], NULL, 10); //TODO: No error handling
	pauseIt = false;
	sigaddset(&sigSet, SIGCHLD);
	sigaddset(&sigSet, SIGCONT);
	sigaddset(&sigSet, SIGURG);
	sigprocmask(SIG_UNBLOCK, &sigSet, NULL);

	/*siga.sa_sigaction = *multi_handler;
	siga.sa_flags |= SA_SIGINFO;

	sigaction(SIGCONT, &siga, NULL);*/

	while (remainingtime > 0) {
		//TODO: Linux is trash and so am I
		sleep(1);
		cout << "Proc " << getpid() << ": TICK TICK" << endl;
		remainingtime--;
	}
	cout << "Proc " << getpid() << ": finished! Returning ..." << endl;
	return 0;
}
