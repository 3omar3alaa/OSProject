#include <stdio.h>
#include <queue>
using namespace std;
#include "headers.h"

//structs
struct process {
	long mtype;
	int arrivaltime;
	int priority;
	int runningTime;
	int id;
	int pid;
};

//Variables
queue<process> processQue;
int quantum;
bool isProcessing;
bool gotNewEvent;
int msgqid;
process* currentProcess;
long lastRun;

//Functions
void startRR();
void handleProcessArrival(int);
void handleClkSignal(int);
void handleChild(int);
void RoundRobinIt();
void unhandleClkSignal(int);
void runProcess(process*);
void ClearResources(int);
void testSendProcess();

int main(int argc, char* argv[]) {
    
	cout << "Scheduler started with algorithm number " << argv[1] << endl;
	signal(SIGINT, ClearResources);
	//initClk(); //DEBUG
	int whichAlgo = *(argv[1]) - '0';

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
	//DEBUG
	testSendProcess();
	if (gotNewEvent)
		RoundRobinIt();
	/*while (true) {
		pause();
		cout << "Resuming" << endl;
		while(gotNewEvent)
			RoundRobinIt();
	}*/
    
}

void startRR() {
	quantum = 2;
	isProcessing = false;
	gotNewEvent = false;
	lastRun = 0;
	cout << "Starting RR Algorithm with a quantum of " << quantum << endl;

	msgqid = msgget(MSGQKEY, IPC_CREAT | 0644);
	if (msgqid == -1) {
		cout << "Error in getting message queue, Source: Scheduler" << endl;
		exit(-1);
	}

	signal(SIGCONT, handleProcessArrival); //TODO: what signal for handling process arrival?
	cout << "Starting handling process arrival signal" << endl;	

	signal(SIGCHLD, handleChild);		 
	cout << "Starting handling child" << endl;
}

void handleProcessArrival(int dumbLinux) {
	//TODO: lock queue
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
			arrivedProcess.pid = -1;
			processQue.push(arrivedProcess);
			if (!isProcessing) {
				cout << "Waking up RoundRobinIt!"<<endl;
				gotNewEvent = true;
			}
		}
		firstMsg = false;
	}
}

void handleClkSignal(int dumbLinux) {
	//TODO: lock queue
	//TODO: Not efficient; calls not per quantum
	gotNewEvent = true;
}

void handleChild(int dumbLinux) {
	//TODO: lock queue
	int childPid;
	int   status;
	//TODO: this assumes dead child is currentProcess
	if ((childPid = waitpid(-1, &status, WNOHANG)) != -1)
	{
		cout << "Got signal for child death :( with pid " << childPid << endl;
		gotNewEvent = true;
		delete(currentProcess);
	}
}

void RoundRobinIt() {
	//TODO: lock queue	
	if (gotNewEvent) {
		gotNewEvent = false; //TODO: RACE CONDITION
		//bool finishedQuantum = getClk() - lastRun >= quantum;
		bool finishedQuantum = false;//DEBUG
		if (processQue.empty() && currentProcess == NULL) {
			cout << "Queue empty in scheduler. Unhandling clock ... " << endl;
			signal(SIGCONT, unhandleClkSignal); //TODO: insert correct handled signal, unhandle signal correctly
			isProcessing = false;
			return;
		}
		if (!isProcessing) {
			cout << "Starting handling clock signal" << endl;
			signal(SIGCONT, handleClkSignal); //TODO: insert correct handled signal
			isProcessing = true;
		}
		if (currentProcess == NULL) {
			currentProcess = &(processQue.front());
			processQue.pop();
			cout << "Dequeued process with id " << currentProcess->id << endl;
			runProcess(currentProcess);
			//lastRun = getClk(); //DEBUG
		}
		else if (finishedQuantum) {
			kill(currentProcess->pid, SIGSTOP);
			cout << "Stopped process with id " << currentProcess->id << endl;
			processQue.push(*currentProcess);
			currentProcess = &(processQue.front());
			processQue.pop();
			cout << "Dequeued process with id " << currentProcess->id << endl;
		}
	}	
}

void runProcess(process* runThis) {
	if (runThis->pid == -1) {
		cout << "Forking new process for the first time with id " << runThis->id << endl;
		char pTime[100]; //TODO size?
		sprintf(pTime, "%d", runThis->runningTime);
		cout << pTime<<endl;//DEBUG
		int pid = fork(); //TODO: if error
		if (pid == 0) {
			execl("./process.out", pTime, (char*)0);
		}
		else
			runThis->pid = pid;

		cout << "Ran a process with pid " << runThis->pid << endl;
	}
	else {
		kill(runThis->pid, SIGCONT);
		cout << "Woke up process with id " << runThis->id << endl;
	}
}

void unhandleClkSignal(int dumbLinux) {	//empty because I don't want to handle it 
}

void ClearResources(int) {
	//destroyClk(false);
	//TODO: clear pointers?
}

void testSendProcess() {
	int send_val;
	struct process toSendProcess;

	toSendProcess.mtype = 7;     	
	toSendProcess.arrivaltime = 3;
	toSendProcess.id = 1;
	toSendProcess.pid = -1;
	toSendProcess.priority = 1;
	toSendProcess.runningTime = 5;
	int sendSize = sizeof(toSendProcess) - sizeof(toSendProcess.mtype);
	send_val = msgsnd(msgqid, &toSendProcess, sendSize, !IPC_NOWAIT);

	if (send_val == -1)
		cout<<"Errror in send"<<endl;
	else
		cout << "Sent a process"<<endl;

	cout << "Sending signal to self" << endl;
	kill(getpid(), SIGCONT);
	cout << "Sent a signal" << endl;

}