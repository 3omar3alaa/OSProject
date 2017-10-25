#include "headers.h"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;
void ClearResources(int);
void CountProcesses(int* count);
void ConfigureTimeArrivalArray(int* arr);
void ConfigureTimeArrivalArray(int* arr,int* index);
struct process{
	int id;
	int pid;
	int priority;
	int arrivalTime;
	int runtime;
};
void Check(std::vector<std::vector<process> > &processesVector);
void BuildStruct(struct process *p,string line);

int main() {
    
	signal(SIGINT,ClearResources);
	//Here implement the reading of process.txt
	int count = 0;
	int index = 0;
	//CountProcesses(&count);
	//ConfigureTimeArrivalArray(arrivalTimeArr,& index);
	std::vector< std::vector<process> > processesVector;
	Check(processesVector);
	cout<<"The size of processesVector is "<<processesVector.size()<<"\n";
	int* arrivalTimeArr = new int[processesVector.size()];
	for(int i =0;i<processesVector.size();i++)
	{
		arrivalTimeArr[i] = processesVector[i][0].arrivalTime;
	}
	cout<<"The arrivalTimeArr elements are \n";
	for(int i =0;i<processesVector.size();i++)
	{
		cout<< arrivalTimeArr[i]<<"\n";
	}
	//cout<< v[0] <<"\n";
	//TODO: 
	// 1-Ask the user about the chosen scheduling Algorithm and its parameters if exists.
	// 2-Initiate and create Scheduler and Clock processes.


	// 3-use this function after creating clock process to initialize clock
	initClk();
	/////Toget time use the following function
	int x= getClk();
	printf("current time is %d\n",x);
	//TODO:  Generation Main Loop
	//4-Creating a data structure for process  and  provide it with its parameters 
	//5-Send the information to  the scheduler at the appropriate time 
	//(only when a process arrives) so that it will be put it in its turn.

	//6-clear clock resources
	destroyClk(true);

}

void ClearResources(int)
{
	//TODO: it clears all resources in case of interruption
	exit(0);
}


void Check(std::vector<std::vector<process> > &processesVector)
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
		cout<<"The file is opened correctly\n";
		while(getline(in,line))
		{
			flag = 0;
			cout<<"The line is " << line <<"\n";
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