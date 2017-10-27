#include <cstdlib>
using namespace std;
#include "headers.h"

//Variables
long remainingtime;

//TODO: handle SIGCONT, SIGSTOP
int main(int agrc, char* argv[]) {
	cout << "Process started with running time of " << argv[0] << endl;
	//initClk(); //DEBUG
	remainingtime = strtol(argv[0],NULL,10); //TODO: No error handling
	//DEBUG
	cout << remainingtime << endl;

	/*while (remainingtime>0) {
		sleep(1);
		remainingtime--;
	}*/

	//destroyClk(false);//DEBUG
	return 0;
}
