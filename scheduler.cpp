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
sigset_t sigSet;

//Functions
void startRR();
void handleProcessArrival(int);
void handleClkSignal(int);
void handleChild(int);
void RoundRobinIt();
void unhandleClkSignal(int);
void runProcess(processI*);
void ClearResources(int);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    
	cout << "Scheduler started with algorithm number " << argv[0] << " and pid of " << getpid() << endl;
	signal(SIGINT, ClearResources);
	int whichAlgo = *(argv[0]) - '0';

	switch (whichAlgo) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		startRR();
		break;
	default:
		cout << "Error! No matching scheduling algorthim to no. " << whichAlgo << endl;
		exit(-1); //TODO: handle this?
		break;
	}
	while (true) {
		pause();
		while(gotNewEvent)
			RoundRobinIt();
	}
    
}

void startRR() {
	quantum = 4;
	isProcessing = false;
	gotNewEvent = false;
	lastRun = 0;
	deletePid = 0;
	initClk();
	cout << "Starting RR Algorithm with a quantum of " << quantum << endl;

	sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGCHLD);
	sigaddset(&sigSet, SIGURG);
	sigaddset(&sigSet, SIGCONT);

	msgqid = msgget(MSGQKEY, IPC_CREAT | 0644);
	if (msgqid == -1) {
		cout << "Error in getting message queue, Source: Scheduler" << endl;
		exit(-1);
	}

	signal(SIGURG, handleProcessArrival);
	cout << "Starting handling process arrival signal" << endl;	//TODO: shouldn't work

	signal(SIGCHLD, handleChild);		 
	cout << "Starting handling child" << endl;
}

void handleProcessArrival(int dumbLinux) {
	//TODO: lock queue
	//TODO: Make it SIGCONT and diff signaler
	cout << "Got a signal of process arrival" << endl;
	int recVal = 0;
	bool firstMsg = true;
	struct process arrivedProcess;
	int recSize = sizeof(arrivedProcess) - sizeof(arrivedProcess.mtype);

	while (recVal != -1) {
		recVal = msgrcv(msgqid, &arrivedProcess, recSize, 0, IPC_NOWAIT);
		if (recVal == -1 && firstMsg) {
			cout << "Error in receiving arrived process from PG! Skipping .." << endl;
		}
		else if (recVal != -1){
			cout << "Received an arrived process with id of " << arrivedProcess.id << endl;
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

void handleClkSignal(int dumbLinux) {
	//TODO: lock queue
	//TODO: Not efficient; calls not per quantum
	cout << "SCH: Received signal from clock" << endl; 
	gotNewEvent = true;
}

void handleChild(int dumbLinux) {
	//TODO: lock queue
	int childPid;
	int   status;
	cout << "PAM" << endl;
	//TODO: this assumes single dead child
	if ((childPid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		cout << "UNPAM" << endl;
		if (WIFEXITED(status)) {
			cout << "Got signal for child death with pid " << childPid << endl;
			gotNewEvent = true;
			deletePid = childPid; 
		}
	}
}

void RoundRobinIt() {
	//TODO: lock queue	
	sigprocmask(SIG_BLOCK, &sigSet, NULL);
	cout << "Starting RoundRobinIt" << endl;
	if (gotNewEvent) {
		gotNewEvent = false; //TODO: RACE CONDITION
		bool finishedQuantum = getClk() - lastRun >= quantum;
		if (deletePid != 0) {
			if (currentProcess->pid == deletePid) {
				cout << "Deleting process with id " << currentProcess->id << " and pid " << currentProcess->pid << endl;
				currentProcess = NULL; //TODO: delete?
			}				
			else {
				for (int i = 0; i < processQue.size(); i++) {
					if (processQue.at(i).pid == deletePid) {
						cout << "XDeletingX process with id " << processQue.at(i).id << " and pid " << processQue.at(i).pid << endl;
						processQue.erase(processQue.begin() + i);
						break;
					}
				}
			}
			deletePid = 0;
		}
		if (processQue.empty() && currentProcess == NULL) {
			cout << "Queue empty in scheduler. Unhandling clock ... " << endl;
			signal(SIGCONT, unhandleClkSignal);
			isProcessing = false;
			return;
		}
		if (!isProcessing) {
			cout << "Starting handling clock signal" << endl;
			signal(SIGCONT, handleClkSignal);
			isProcessing = true;
		}
		if (currentProcess == NULL) {
			currentProcess = &(processQue.front());
			processQue.pop_front();
			cout << "Dequeued process with id " << currentProcess->id << " and pid of " << currentProcess->pid << endl;
			runProcess(currentProcess);
			lastRun = getClk(); 
		}
		else if (finishedQuantum) {
			kill(currentProcess->pid, SIGTSTP);
			cout << "Stopped process with id " << currentProcess->id << " and pid of " << currentProcess->pid << endl;
			processQue.push_back(*currentProcess);
			currentProcess = &(processQue.front());
			processQue.pop_front();
			cout << "Dequeued process with id " << currentProcess->id << " and pid of " << currentProcess->pid << endl;
			runProcess(currentProcess);
			lastRun = getClk();
		}
	}	
	sigprocmask(SIG_UNBLOCK, &sigSet, NULL);
}

void runProcess(processI* runThis) {
	if (runThis->pid == -1) {
		char pTime[100]; //TODO size?
		sprintf(pTime, "%d", runThis->runningTime);
		int pid = fork(); //TODO: if error
		if (pid == 0) {
			execl("./process.out", pTime, (char*)0);
		}
		else
			runThis->pid = pid;

		cout << "Ran a process with id " << runThis->id << " and pid " << runThis->pid << endl;
	}
	else {
		kill(runThis->pid, SIGCONT);
		cout << "Woke up process with id " << runThis->id << " and pid of " << runThis->pid << endl;
	}
}

void unhandleClkSignal(int dumbLinux) {	//empty because I don't want to handle it 
}

void ClearResources(int) {
	cout << "SCH: Clearing resources.." << endl;
	destroyClk(false);
	raise(9);
	//TODO: clear pointers?
}