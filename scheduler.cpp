#include <stdio.h>
#include <iomanip> 
#include <queue>
#include <sstream>
#include <fstream>
#include <cmath>
#include <list>
#include <algorithm>
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
list <processI> SRTNprocesslist;
stringstream stats;
long remainingtime;
key_t PrcmsgQId;
int whichAlgo;
int quantum;
bool isProcessing;
bool gotNewEvent;
int msgqid;
//changed current process to NULL
processI* currentProcess;
long lastRun;
int deletePid;
sigset_t ssForNoINT;
double sumPT;
double sumWait;
double sumWTA;
double nP;
vector<double> WTAs;

//Functions
void calculateRemTime(processI* p);
void handleProcessArrival(int);
void handleClkSignal(int);
void handleChild(int);
void ClearResources(int);
void startSRTN();
void startRR();
void RoundRobinIt();
bool runProcess(processI*);
void log(processI*, logType);
void SRTNIt();
bool haslessProcessTime(processI p1, processI p2);
void startHPF();
bool priority(processI, processI);
void HPFIt();

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    
	cout << "SCH: Starting with algorithm number " << argv[0] << " and pid of " << getpid() << endl;
	signal(SIGINT, ClearResources); sumPT = nP = sumWait = 0;
	cout << setprecision(2);
	stats << setprecision(2);
	stats << "#At time x process y state arr w total z remain y wait k\n";

	whichAlgo = *(argv[0]) - '0';

	switch (whichAlgo) {
	case 1:
		startHPF();
		while (true) {
			pause();
			while (gotNewEvent)
				HPFIt();
		}
		break;
	case 2:
		startSRTN();
		while(true)
		{
			pause();
			while(gotNewEvent)
				SRTNIt();
		}
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

void startHPF() {
	isProcessing = false;
	gotNewEvent = false;
	lastRun = 0;
	deletePid = 0;
	initClk();
	cout << "Starting Non-Preemptive Highest Priority First Algorithm \n";

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

void startSRTN()
{
	isProcessing = false;
	gotNewEvent = false;
	lastRun = 0;
	deletePid = 0;
	initClk();
	cout << "SCH: Starting SRTN Algorithm\n";

	msgqid = msgget(MSGQKEY, IPC_CREAT | 0644);
	if (msgqid == -1) {
		cout << "SCH: Error in getting message queue, Source: Scheduler" << endl;
		exit(-1);
	}

	signal(SIGCONT, handleProcessArrival);
	cout << "SCH: Starting handling process arrival signal" << endl;
	signal(SIGCHLD, handleChild);		 
	cout << "SCH: Starting handling child" << endl;
}

void startRR() {
	quantum = 4;
	isProcessing = false;
	gotNewEvent = false;
	lastRun = 0;
	deletePid = 0;
	initClk();
	cout << "SCH: Starting RR Algorithm " << endl;

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
			nP++;
			cout << "SCH: Received an arrived process with id of " << arrivedProcess.id << endl;
			struct processI newProcess;
			newProcess.pid = -1;
			newProcess.arrivalTime = arrivedProcess.arrivalTime;
			newProcess.id = arrivedProcess.id;
			newProcess.mtype = arrivedProcess.mtype;
			newProcess.priority = arrivedProcess.priority;
			sumPT += newProcess.processTime = newProcess.remTime = arrivedProcess.runtime;
			if (whichAlgo == 1) {
				processQue.push_back(newProcess);
				if (!processQue.empty())
				sort(processQue.begin(), processQue.end(), priority);
			}
			else if (whichAlgo == 2)
				SRTNprocesslist.push_back(newProcess);
			else if(whichAlgo == 3)
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

void HPFIt() {
	sigprocmask(SIG_BLOCK, &ssForNoINT, NULL);
	cout << "SCH: Starting HPFIt" << endl;
	if (gotNewEvent) {
		gotNewEvent = false;
		if (deletePid > 0) {
			if (currentProcess->pid == deletePid) {
				cout << "SCH: Deleting process with id " << currentProcess->id << " and pid " << currentProcess->pid << endl;
				log(currentProcess, Finished);
				currentProcess = NULL;
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
			}
		}
	}
	sigprocmask(SIG_UNBLOCK, &ssForNoINT, NULL);
}

bool haslessProcessTime(processI p1, processI p2)
{
	return p1.processTime < p2.processTime; 
}

void calculateRemTime(processI* p)
{
	p-> remTime = p->remTime - (getClk() - lastRun);
	lastRun = getClk();
}

void SRTNIt()
{
	gotNewEvent = false;
	if(!SRTNprocesslist.empty())
	{
		if(currentProcess != NULL)
		{
			if (currentProcess->pid == deletePid)
			{
				cout<<"SCH: Deleting the front process\n";
				log(currentProcess,Finished);
				SRTNprocesslist.pop_front();
				if(!SRTNprocesslist.empty())
				{
					currentProcess = &SRTNprocesslist.front();
					runProcess(currentProcess);
					if(currentProcess->processTime == currentProcess ->remTime)
						{
							log(currentProcess,Started);
							cout <<"SCH: Started Process of id "<< currentProcess -> id <<" and process time "<< currentProcess-> remTime <<"\n";
						}
					else 
						{
							log(currentProcess,Resumed);
							cout <<"SCH: Resumed Process of id "<< currentProcess -> id <<" and remaining time "<< currentProcess-> remTime <<"\n";
						}
					lastRun = getClk();
				}
			}
			else 
			{
				calculateRemTime(currentProcess);
				cout<<"SCH: The remaining time of the current Process is "<< currentProcess->remTime <<endl;
				SRTNprocesslist.sort(haslessProcessTime);

				if(currentProcess->pid != SRTNprocesslist.front().pid)
				{
					kill(currentProcess->pid, SIGTSTP);
					log(currentProcess,Stopped);
					currentProcess = &SRTNprocesslist.front();
					runProcess(currentProcess);
					if(currentProcess->processTime == currentProcess ->remTime)
					{
						log(currentProcess,Started);
						cout <<"SCH: Started Process of id "<< currentProcess -> id <<" and process time "<< currentProcess-> remTime <<"\n";
					}
					else 
					{
						log(currentProcess,Resumed);
						cout <<"SCH: Resumed Process of id "<< currentProcess -> id <<" and remaining time "<< currentProcess-> remTime <<"\n";
					}
				}
				else 
				{
					cout << "SCH: The currentProcess is still running and has remaining time of "<<currentProcess->remTime <<"\n";
				}
			}
		
		}
		else
		{
			SRTNprocesslist.sort(haslessProcessTime);
			currentProcess = &SRTNprocesslist.front();
			runProcess(currentProcess);
			lastRun = getClk();
			if(currentProcess->processTime == currentProcess ->remTime)
			{
				log(currentProcess,Started);
				cout <<"SCH: Started Process of id "<< currentProcess -> id <<" and process time "<< currentProcess-> remTime <<"\n";
			}
			else 
			{
				log(currentProcess,Resumed);
				cout <<"SCH: Resumed Process of id "<< currentProcess -> id <<" and remaining time "<< currentProcess-> remTime <<"\n";
			}
		}
		
	}
	else
		currentProcess = NULL;
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
	double now = getClk();
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
	{
		double wait = ((now - p->arrivalTime) - (p->processTime));
		double WTA = (now - p->arrivalTime) / p->processTime;
		sumWait += wait; sumWTA += WTA;
		WTAs.push_back(WTA);
		stats << "At time " << now << " process " << p->id << " finished arr " << p->arrivalTime << " total " << p->processTime << " remain 0 " << " wait " << wait << " TA " << (now - p->arrivalTime) << " WTA " << WTA << "\n";
		break;
	}
	default:
		cout  << "SCH: Logging error!" << endl;
		stats << "Logging error!" << endl;
		break;
	}
}

void ClearResources(int) {
	double t = getClk(); //TODO: make it for sure
	cout << "SCH: Logging data to file" << endl; 
	ofstream outL("scheduler.log");
	outL << stats.str();
	outL.close(); 

	cout << "SCH: Calculating performance" << endl;
	ofstream outP("scheduler.perf");
	outP << setprecision(2)<<"CPU utilization=" << ((sumPT / t) * 100) << "%" << endl;
	outP << "Avg WTA=" << sumWTA / nP << endl;
	outP << "Avg Waiting=" << sumWait / nP << endl;
	int sigma = 0;
	for (int i = 0; i < WTAs.size(); i++) {
		sigma += pow(2, WTAs.at(i) - sumWTA/nP);
	}
	outP << "Std WTA=" << sqrt(sigma / nP) << endl;
	outP.close();

	cout << "SCH: Clearing resources.." << endl;
	destroyClk(false);
	raise(9);
}

bool priority(processI px1, processI px2) {
	if (px1.priority == px2.priority)
	{
		if (px1.processTime != px2.processTime)
			return (px1.processTime < px2.processTime);
		else if (px1.arrivalTime != px2.arrivalTime)
			return (px1.arrivalTime < px2.arrivalTime);
		else
			return (px1.id < px2.id);
	}
	else
		return  (px1.priority > px2.priority);
}