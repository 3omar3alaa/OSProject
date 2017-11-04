#include <stdio.h>
#include <iomanip> 
#include <queue>
#include <sstream>
#include <fstream>
#include <cmath>
#include <list>
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
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    
	cout << "SCH: Starting with algorithm number " << argv[0] << " and pid of " << getpid() << endl;
	signal(SIGINT, ClearResources); sumPT = nP = sumWait = 0;
	cout << setprecision(2);
	stats << setprecision(2);
	stats << "#At time x process y state arr w total z remain y wait k\n";

	PrcmsgQId = msgget(PRCMSGQKEY, IPC_CREAT|0644);
	if(PrcmsgQId == -1)
	{	
	    perror("Error in create");
	    exit(-1);
	}
	else 
	{
		cout << "SCH: The process message queue was initizalized correctly\n";
	}
	whichAlgo = *(argv[0]) - '0';

	switch (whichAlgo) {
	case 1:
		break;
	case 2:
		startSRTN();
		while(true)
		{
			pause();
			cout<<"SCH: Woke up from pause\n";
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
			if(whichAlgo == 2)
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
	cout<<"SCH: Hello from SIGCHLD handler\n";
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

bool haslessProcessTime(processI p1, processI p2)
{
	return p1.processTime < p2.processTime; 
}

void SRTNIt()
{
	gotNewEvent = false;
	cout<<"SCH: SRTNIt started\n";
	if(!SRTNprocesslist.empty())
	{
		cout<<"SCH: SRTNprocesslist is NOT Empty\n";
		if(currentProcess != NULL)
		{
			if (currentProcess->pid == deletePid)
			{
				cout<<"SCH: Deleting the front process and starting another\n";
				SRTNprocesslist.pop_front();
				currentProcess = &SRTNprocesslist.front();
				runProcess(currentProcess);
				cout <<"SCH: Process of id "<< currentProcess -> id <<" has started\n";
			}
			else 
			{
				kill(currentProcess->pid, SIGURG);
				struct pTime p;
				int recVal = msgrcv(PrcmsgQId, &p, sizeof(long), 0, !IPC_NOWAIT);
				if(recVal == -1)
					cout<<"SCH: Error in receiving the message from the process\n";
				else
					{
						currentProcess->processTime = p.remainingtime;
						cout<<"SCH: The remainingtime of the current Process is "<<currentProcess->processTime<<endl;
					}

				SRTNprocesslist.sort(haslessProcessTime);

				if(currentProcess->pid != SRTNprocesslist.front().pid)
				{
					kill(currentProcess->pid, SIGTSTP);
					currentProcess = &SRTNprocesslist.front();
					runProcess(currentProcess);
				}
				else 
				{
					cout << "SCH: The currentProcess is still running\n";
				}
			}
		
		}
		else
		{
			SRTNprocesslist.sort(haslessProcessTime);
			cout<<"SCH: Entered in else condition\n";
			currentProcess = &SRTNprocesslist.front();
			runProcess(currentProcess);
			cout <<"SCH: Process of id "<< currentProcess -> id <<" has started\n";
		}
		
	}
	cout<<"SCH: SRTNIt ended\n";
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
	msgctl(PrcmsgQId, IPC_RMID, (struct msqid_ds *) 0);
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