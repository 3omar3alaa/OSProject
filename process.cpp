#include <cstdlib>
using namespace std;
#include "headers.h"

//Variables
long remainingtime;

//TODO: handle SIGCONT, SIGSTOP
int main(int agrc, char* argv[]) {
	cout << "Proc "<<getpid()<<": Starting with running time of " << argv[0] << endl;
	initClk();
	remainingtime = strtol(argv[0], NULL, 10); //TODO: No error handling

	while (remainingtime>0) {
		sleep(1);
		remainingtime--;
	}
	cout << "Proc " << getpid() << ": finished! Returning ..." << endl;
	destroyClk(false);
	return 0;
}
