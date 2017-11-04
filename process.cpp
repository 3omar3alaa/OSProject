#include <cstdlib>
using namespace std;
#include "headers.h"

//Variables
key_t PrcmsgQId;
long remainingtime;
long lastRun = -1;
sigset_t sigSet;
struct sigaction siga;

//Functions
void recordStart(int) {
	lastRun = getClk();
	cout << "PRC " << getpid() << ": lastRun now is " << lastRun << endl;
}

void recordEnd(int) {
	remainingtime -= getClk() - lastRun;
	cout << "PRC " << getpid() << ": remTime now is " << remainingtime << endl;
	if (remainingtime < 1) {
		cout << "PRC " << getpid() << ": finished! Returning ..." << endl;
		destroyClk(false);
		exit(0);
	}
	else
		raise(SIGSTOP);
}

void sendRemTime(int)
{
	cout<<"PRC: SIGURG Handler\n";
	pTime p;
	p.remainingtime = remainingtime - (getClk() - lastRun);
	cout<<"PRC: lastRun is "<<lastRun <<" getClk is "<< getClk() << " remainingtime is "<<remainingtime<<endl;
	int send_val = msgsnd(PrcmsgQId, &p, sizeof(p)-sizeof(p.mtype), IPC_NOWAIT);
	if(send_val == -1)
		cout<<"PRC: Error in sending the remainingtime to the Scheduler\n";
}
//////////////////////////////////////////////////////////////////

int main(int agrc, char* argv[]) {

	initClk();
	lastRun = getClk();
	cout << "PRC " << getpid() << ": Starting with running time of " << argv[0] << endl;
	remainingtime = strtol(argv[0], NULL, 10); //TODO: No error handling
	//Getting the message queue initiated by the Scheduler 
	PrcmsgQId = msgget(PRCMSGQKEY, IPC_CREAT|0644);
	if(PrcmsgQId == -1)
	{	
	    cout<<"Error in create\n";
	    exit(-1);
	}

	
	//remove blocking of signals done by parent
	sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGCHLD);
	sigaddset(&sigSet, SIGCONT);
	sigaddset(&sigSet, SIGURG);
	sigprocmask(SIG_UNBLOCK, &sigSet, NULL);

	signal(SIGURG, sendRemTime);
	signal(SIGCONT, recordStart);
	signal(SIGTSTP, recordEnd);

	while (remainingtime > (getClk() - lastRun)) {
		sleep(1);//TODO: not in sync
		cout << "PRC " << getpid() << ": TICK TICK" << endl;
	}
	cout << "PRC " << getpid() << ": finished! Returning ..." << endl;
	destroyClk(false);
	return 0;
}
