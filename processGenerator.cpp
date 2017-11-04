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
//void CountProcesses(int* count);
//void ConfigureTimeArrivalArray(int* arr);
//void ConfigureTimeArrivalArray(int* arr,int* index);
void Build2DVector(std::vector<std::vector<process> > &processesVector);
void BuildStruct(struct process *p,string line);

int *arrivalTimeArr = NULL;
int currentArrivalIndex = 0;
int clkId = 0;
int schId = 0;
key_t msgQId;
std::vector< std::vector<process> > processesVector;
bool wakeUpSch;

int main() {
	wakeUpSch = false;
	
	//Here implement the reading of process.txt
	int count = 0;
	int index = 0;
	int scheduler = 0;
	Build2DVector(processesVector);
	//cout<<"The size of processesVector is "<<processesVector.size()<<"\n";
	arrivalTimeArr = new int[processesVector.size()];
	for(int i =0;i<processesVector.size();i++)
	{
		arrivalTimeArr[i] = processesVector[i][0].arrivalTime;
	}
	//cout<<"The arrivalTimeArr elements are \n";
	/*for(int i =0;i<processesVector.size();i++)
	{
		cout<< arrivalTimeArr[i]<<"\n";
	}*/
	//cout<< v[0] <<"\n";
	//TODO: 
	// 1-Ask the user about the chosen scheduling Algorithm and its parameters if exists.
	cout<<"Please choose the scheduler algorithm you want\n";
	cout<<"1. non-preemptive HPF\n2. Shortest Remaining time Next\n3. Round Robin\n";
	cin>>scheduler;
	//cout<<"The pgen id is "<<getpid()<<"\n";
	//cout<<"The pgen grp id is "<<getpgrp()<<"\n";
	// 2-Initiate and create Scheduler and Clock processes.
	//Initiating the Scheduler
	schId = fork();
	if(schId == 0)
	{
		char p[10];
		sprintf(p,"%d",scheduler);
		execl("sch.out",p, (char *) 0);
	}
	//Initiating the clock
	else
	{
		clkId = fork();
	}

	if(clkId == 0)
	{
		char p[10];
		sprintf(p,"%d",schId);
		execl("clock.out",p, (char *) 0);
		//cout<<"The parent id is "<<getppid()<<"\n";
	}
	else
	{
		msgQId = msgget(MSGQKEY, IPC_CREAT|0644);
		if(msgQId == -1)
		{	
		    perror("Error in create");
		    exit(-1);
		}
		// 3-use this function after creating clock process to initialize clock
		initClk();
		/////Toget time use the following function
		int x= getClk();
		//printf("current time is %d\n",x);
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
			//cout<<"Returned from pause"<<endl;
		}
		//while(1){}
		//TODO:  Generation Main Loop
		//4-Creating a data structure for process  and  provide it with its parameters 
		//5-Send the information to  the scheduler at the appropriate time 
		//(only when a process arrives) so that it will be put it in its turn.

		//6-clear clock resources
		//destroyClk(true);
	}

}

void ClearResources(int)
{
	//TODO: it clears all resources in case of interruption
	cout<<"Clearing resources ..." << endl;
	destroyClk(true);
	raise(9);
}

void ClockChanged(int)
{
	int send_val;
	cout<<"PGEN: the current time is "<<getClk()<<".............................\n";
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
		//cout<<"The file is opened correctly\n";
		while(getline(in,line))
		{
			flag = 0;
			//cout<<"The line is " << line <<"\n";
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