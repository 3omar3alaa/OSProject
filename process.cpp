#include <cstdlib>
using namespace std;
#include "headers.h"

//Variables
long remainingtime;
sigset_t sigSet;
bool pauseIt;


int main(int agrc, char* argv[]) {
	cout << "Proc "<<getpid()<<": Starting with running time of " << argv[0] << endl;
	remainingtime = strtol(argv[0], NULL, 10); //TODO: No error handling
	pauseIt = false;
	sigaddset(&sigSet, SIGCHLD);
	sigaddset(&sigSet, SIGCONT);
	sigaddset(&sigSet, SIGURG);
	sigprocmask(SIG_UNBLOCK, &sigSet, NULL);

	while (remainingtime>0) {
		//TODO: Linux is trash and so am I
		sleep(1);
		cout << "Proc " << getpid() << ": TICK TICK" << endl;
		remainingtime--;
	}
	cout << "Proc " << getpid() << ": finished! Returning ..." << endl;
	return 0;
}
