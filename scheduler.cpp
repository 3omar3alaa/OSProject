#include <stdio.h>
#include <queue>
#include <sstream>
#include <fstream>
using namespace std;
#include "headers.h"

//Structs & Enums
struct processI {
	long mtype;
	int arrivalTime;
	int priority;
	int processTime;
	int remTime;
	int id;
	int pid;
};
enum logType {
	Started,
	Stopped,
	Resumed,
	Finished
};

//Variables
deque<processI> processQue;
stringstream stats;
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
void handleChild(int);
void ClearResources(int);
void startRR();
void RoundRobinIt();
bool runProcess(processI*);
void log(processI*, logType);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    
	cout << "SCH: Starting with algorithm number " << argv[0] << " and pid of " << getpid() << endl;
	signal(SIGINT, ClearResources);

	stats << "#At time x process y state arr w total z remain y wait k\n\n";

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
			newProcess.processTime = newProcess.remTime = arrivedProcess.runtime;
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
				log(currentProcess, Finished);
				currentProcess = NULL;
			}				
			else {
				for (int i = 0; i < processQue.size(); i++) {
					if (processQue.at(i).pid == deletePid) {
						cout << "SCH: XDeletingX process with id " << processQue.at(i).id << " and pid " << processQue.at(i).pid << endl;
						log(&processQue.at(i), Finished);
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
					currentProcess->remTime -= quantum;
					cout << "SCH: Stopped process with id " << currentProcess->id << " and pid of " << currentProcess->pid << endl;
					log(currentProcess, Stopped);
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
		sprintf(pTime, "%d", runThis->processTime);
		int pid = fork();
		if (pid == -1) {
			cout << "SCH: Error while trying to fork process with id " << runThis->id << endl;
			return false;
		}
		if (pid == 0) 
			execl("./process.out", pTime, (char*)0);
		else {
			runThis->pid = pid;
			log(runThis, Started);
		}	

		cout << "SCH: Ran a process with id " << runThis->id << " and pid " << runThis->pid << endl;
	}
	else {
		kill(runThis->pid, SIGCONT);
		log(runThis, Resumed);
		cout << "SCH: Woke up process with id " << runThis->id << " and pid of " << runThis->pid << endl;
	}
	return true;
}

void log(processI* p, logType lt) {
	int now = getClk();
	switch (lt) {
	case Started:
		stats << "At time " << now << " process " << p->id << " started arr " << p->arrivalTime << " total " << p->processTime << " remain " << p->remTime << " wait " << ((now - p->arrivalTime) - (p->processTime - p->remTime)) << "\n";
		break;
	case Stopped:
		stats << "At time " << now << " process " << p->id << " stopped arr " << p->arrivalTime << " total " << p->processTime << " remain " << p->remTime << " wait " << ((now - p->arrivalTime) - (p->processTime - p->remTime)) << "\n";
		break;
	case Resumed:
		stats << "At time " << now << " process " << p->id << " resumed arr " << p->arrivalTime << " total " << p->processTime << " remain " << p->remTime << " wait " << ((now - p->arrivalTime) - (p->processTime - p->remTime)) << "\n";
		break;
	case Finished:
		stats << "At time " << now << " process " << p->id << " finished arr " << p->arrivalTime << " total " << p->processTime << " remain 0 " << " wait " << ((now - p->arrivalTime) - (p->processTime)) << " TA " << (now - p->arrivalTime) << " WTA " << (now - p->arrivalTime) / p->processTime << "\n";
		break;
	default:
		cout  << "SCH: Logging error!" << endl;
		stats << "Logging error!" << endl;
		break;
	}
}

void ClearResources(int) {
	cout << "SCH: Logging data to file" << endl; 
	ofstream out("scheduler.log");
	out << stats.str();
	out.close(); 

	cout << "SCH: Clearing resources.." << endl;
	destroyClk(false);
	raise(9);
}