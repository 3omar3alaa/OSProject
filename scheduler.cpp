#include <stdio.h>
#include <queue>
using namespace std;
#include "headers.h"

//Structs
struct processI {
	long mtype;
	int arrivalTime;
	int priority;
	int runningTime;
	int id;
	int pid;
};

//Variables
deque<processI> processQue;
int quantum;
bool isProcessing;
bool gotNewEvent;
int msgqid;
processI* currentProcess;
long lastRun;
int deletePid;
sigset_t ssForNoINT;

//Functions
void handleProcessArrival(int);
void handleClkSignal(int);
void handleChild(int)
void ClearResources(int);
void startRR();
void RoundRobinIt();
bool runProcess(processI*);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    
	cout << "SCH: Starting with algorithm number " << argv[0] << " and pid of " << getpid() << endl;
	signal(SIGINT, ClearResources);
	int whichAlgo = *(argv[0]) - '0';

	switch (whichAlgo) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		startRR();
		while (true) {
			pause();
			while (gotNewEvent) 
				RoundRobinIt();
		}
		break;
	default:
		cout << "SCH: Error! No matching scheduling algorthim to no. " << whichAlgo << endl;
		exit(-1); //TODO: handle this?
		break;
	}    
}

void startRR() {
	quantum = 4;
	isProcessing = false;
	gotNewEvent = false;
	lastRun = 0;
	deletePid = 0;
	initClk();
	cout << "SCH: Starting RR Algorithm with a quantum of " << quantum << endl;

	msgqid = msgget(MSGQKEY, IPC_CREAT | 0644);
	if (msgqid == -1) {
		cout << "SCH: Error in getting message queue, Source: Scheduler" << endl;
		exit(-1);
	}

	signal(SIGCONT, handleProcessArrival);
	cout << "SCH: Starting handling process arrival signal" << endl;
	signal(SIGCHLD, handleChild);		 
	cout << "SCH: Starting handling child" << endl;
	signal(SIGURG, handleClkSignal);

	sigemptyset(&ssForNoINT);
	sigaddset(&ssForNoINT, SIGURG);
	sigprocmask(SIG_BLOCK, &ssForNoINT, NULL);
	sigaddset(&ssForNoINT, SIGCHLD);	
	sigaddset(&ssForNoINT, SIGCONT);
}

void handleProcessArrival(int) {
	cout << "SCH: Got a signal of process arrival" << endl;
	int recVal = 0;
	bool firstMsg = true;
	struct process arrivedProcess;
	int recSize = sizeof(arrivedProcess) - sizeof(arrivedProcess.mtype);

	while (recVal != -1) {
		recVal = msgrcv(msgqid, &arrivedProcess, recSize, 0, IPC_NOWAIT);
		if (recVal == -1 && firstMsg) {
			cout << "SCH: Error in receiving arrived process from PG! Skipping .." << endl;
		}
		else if (recVal != -1){
			cout << "SCH: Received an arrived process with id of " << arrivedProcess.id << endl;
			struct processI newProcess;
			newProcess.pid = -1;
			newProcess.arrivalTime = arrivedProcess.arrivalTime;
			newProcess.id = arrivedProcess.id;
			newProcess.mtype = arrivedProcess.mtype;
			newProcess.priority = arrivedProcess.priority;
			newProcess.runningTime = arrivedProcess.runtime;
			processQue.push_back(newProcess);
			if (!isProcessing) {
				gotNewEvent = true;
			}
		}
		firstMsg = false;
	}
}

void handleClkSignal(int) {
	cout << "SCH: Received signal from clock" << endl; 
	gotNewEvent = true;
}

void handleChild(int) {
	int childPid;
	int   status;
	if ((childPid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		if (WIFEXITED(status)) {
			cout << "SCH: Got signal for child death with pid " << childPid << endl;
			gotNewEvent = true;
			deletePid = childPid; 
		}
	}
}

void RoundRobinIt() {
	sigprocmask(SIG_BLOCK, &ssForNoINT, NULL);
	cout << "SCH: Starting RoundRobinIt" << endl;
	if (gotNewEvent) {
		gotNewEvent = false;
		bool finishedQuantum = getClk() - lastRun >= quantum;
		if (deletePid > 0) {
			if (currentProcess->pid == deletePid) {
				cout << "SCH: Deleting process with id " << currentProcess->id << " and pid " << currentProcess->pid << endl;
				currentProcess = NULL;
			}				
			else {
				for (int i = 0; i < processQue.size(); i++) {
					if (processQue.at(i).pid == deletePid) {
						cout << "SCH: XDeletingX process with id " << processQue.at(i).id << " and pid " << processQue.at(i).pid << endl;
						processQue.erase(processQue.begin() + i);
						break;
					}
				}
			}
			deletePid = 0;
		}
		if (processQue.empty() && currentProcess == NULL) {
			cout << "SCH: Queue empty in scheduler. Blocking clock ... " << endl;
			sigemptyset(&ssForNoINT);
			sigaddset(&ssForNoINT, SIGCHLD);
			sigaddset(&ssForNoINT, SIGCONT);
			isProcessing = false;
		}
		else {
			if (!isProcessing) {
				cout << "SCH: Unblocking clock signal" << endl;
				sigaddset(&ssForNoINT, SIGURG);
				isProcessing = true;
			}
			if (currentProcess == NULL) {
				currentProcess = &(processQue.front());
				processQue.pop_front();
				cout << "SCH: Dequeued process with id " << currentProcess->id << " and pid of " << currentProcess->pid << endl;
				runProcess(currentProcess);
				lastRun = getClk();
			}
			else if (finishedQuantum) {
				if (!processQue.empty()) { 
					kill(currentProcess->pid, SIGTSTP);
					cout << "SCH: Stopped process with id " << currentProcess->id << " and pid of " << currentProcess->pid << endl;
					processQue.push_back(*currentProcess);
					currentProcess = &(processQue.front());
					processQue.pop_front();
					cout << "SCH: Dequeued process with id " << currentProcess->id << " and pid of " << currentProcess->pid << endl;
					if (!runProcess(currentProcess)) {
						gotNewEvent = true;
						processQue.push_back(*currentProcess);
						currentProcess == NULL;
						cout << "SCH: Renqueued process for not able to fork it." << endl;
					}
					lastRun = getClk();
				}
			}
		}		
	}	
	sigprocmask(SIG_UNBLOCK, &ssForNoINT, NULL);
}

bool runProcess(processI* runThis) {
	if (runThis->pid == -1) {
		char pTime[6]; //assuming default max value of PID
		sprintf(pTime, "%d", runThis->runningTime);
		int pid = fork();
		if (pid == -1) {
			cout << "SCH: Error while trying to fork process with id " << runThis->id << endl;
			return false;
		}
		if (pid == 0)
			execl("./process.out", pTime, (char*)0);
		else
			runThis->pid = pid;

		cout << "SCH: Ran a process with id " << runThis->id << " and pid " << runThis->pid << endl;
	}
	else {
		kill(runThis->pid, SIGCONT);
		cout << "SCH: Woke up process with id " << runThis->id << " and pid of " << runThis->pid << endl;
	}
	return true;
}

void ClearResources(int) {
	cout << "SCH: Clearing resources.." << endl;
	destroyClk(false);
	raise(9);
}