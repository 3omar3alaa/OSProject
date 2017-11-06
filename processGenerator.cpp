#include "headers.h"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <sys/ipc.h>

using namespace std;

void ClearResources(int);
void ClockChanged(int);
void Build2DVector(std::vector<std::vector<process> > &processesVector);
void BuildStruct(struct process *p,string line);
void TERMRequest(int);

int *arrivalTimeArr = NULL;
int currentArrivalIndex = 0;
int clkId = 0;
int schId = 0;
key_t msgQId;
std::vector< std::vector<process> > processesVector;
bool wakeUpSch;

int main() {

	wakeUpSch = false;
	
	signal(SIGUSR1, TERMRequest);
	int count = 0;
	int index = 0;
	int scheduler = 0;
	Build2DVector(processesVector);
	arrivalTimeArr = new int[processesVector.size()];
	for(int i =0;i<processesVector.size();i++)
	{
		arrivalTimeArr[i] = processesVector[i][0].arrivalTime;
	}
	cout<<"Please choose the scheduler algorithm you want\n";
	cout<<"1. Non-preemptive HPF\n2. Shortest Remaining Time Next\n3. Round Robin\n";
	cin>>scheduler;
	if(scheduler < 1 || scheduler > 3)
	{
		cout<<"You entered an invalid number\nGoodnight ..... \n";
		exit(-1);
	}
	int quantum;
	if (scheduler == 3) {
		cout << "Enter quantum value: " << endl;
		cin >> quantum;
	}
	schId = fork();
	if(schId == 0)
	{
		
		char p[10];
		sprintf(p,"%d",scheduler);
		if (scheduler == 3) {
			char q[6]; //assuming char not greater than 999999
			sprintf(q, "%d", quantum);
			execl("sch.out", p,q, (char *)0);
		}
		else
			execl("sch.out", p, (char *)0);
	}
	else
	{
		clkId = fork();
	}

	if(clkId == 0)
	{
		char p[10];
		sprintf(p,"%d",schId);
		execl("clock.out",p, (char *) 0);
	}
	else
	{
		msgQId = msgget(MSGQKEY, IPC_CREAT|0644);
		if(msgQId == -1)
		{	
		    perror("Error in create");
		    exit(-1);
		}
		initClk();
		int x= getClk();
		signal(SIGINT, ClearResources);
		signal(SIGURG, ClockChanged);
		while(1)
		{
			pause();
			if (wakeUpSch) {
				wakeUpSch = false;
				kill(schId, SIGCONT);
				cout << "PGEN: Sent signal to " << schId << endl;
			}
		}
	}

}

void ClearResources(int)
{
	cout<<"PGN: Clearing resources ..." << endl;
	if(arrivalTimeArr != NULL) 
		delete arrivalTimeArr;
	msgctl(msgQId, IPC_RMID, (struct msqid_ds *) 0);
	destroyClk();
	kill(schId, SIGINT);
	int status;
	waitpid(-1, &status, 0);
	killpg(getpgrp(), SIGINT);
	exit(0);
}

void ClockChanged(int)
{
	int send_val;
	cout<<"PGEN: The current time is "<<getClk()<<".............................\n";
	if(arrivalTimeArr[currentArrivalIndex]==getClk() && currentArrivalIndex < processesVector.size())
	{
		for(int i=0;i<processesVector[currentArrivalIndex].size();i++)
		{
			send_val = msgsnd(msgQId, &processesVector[currentArrivalIndex][i], sizeof(processesVector[currentArrivalIndex][i])-sizeof(processesVector[currentArrivalIndex][i].mtype), IPC_NOWAIT);
			if(send_val == -1)
	  			perror("Errror in send");
		}
		currentArrivalIndex++;
		wakeUpSch = true;
	}
}

void Build2DVector(std::vector<std::vector<process> > &processesVector)
{
	std::vector<process> temp;
	string comment = "#";
	string line;
	string arrtime ="";
	string prevArrTime ="";
	int flag = 0;
	bool firstline = true;
	ifstream in ("processes.txt");
	if (in.is_open())
	{
		while(getline(in,line))
		{
			flag = 0;
			if(line.find(comment) == string::npos)
			{
				struct process p;
				BuildStruct(&p,line);
				stringstream iss(line);	
				while(getline(iss,arrtime,'\t'))
				{
					if(flag == 1)
					{
						if(firstline)
						{
							prevArrTime = arrtime;
							firstline = false;
						}
						if(prevArrTime.compare(arrtime) == 0)
						{
							temp.push_back(p);
						}
						else 
						{
							processesVector.push_back(temp);
							temp.clear();
							temp.push_back(p);
							prevArrTime = arrtime;
						}
						break;
					}	
					flag++;
				}
			}
	    }
	    processesVector.push_back(temp);
	}
	in.close();
}

void BuildStruct(struct process *p,string line)
{
	string params="";
	p->pid = -2;
	stringstream iss(line);
	int i=0;
	while(getline(iss,params,'\t'))
	{
		if(i==0)
			p->id = atoi(params.c_str());
		else if(i==1)
			p->arrivalTime = atoi(params.c_str());
		else if(i==2)
			p->runtime = atoi(params.c_str());
		else if(i==3)
			p->priority = atoi(params.c_str());
		i++;
	}
}

void TERMRequest(int) {
	if (currentArrivalIndex >= processesVector.size()) {
		raise(SIGINT);
	}
}