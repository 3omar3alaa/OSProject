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
void BuildStruct(struct process *p);
int main() {
    
	signal(SIGINT,ClearResources);
	//Here implement the reading of process.txt
	int count = 0;
	int index = 0;
	CountProcesses(&count);
	cout<<"The number of processes is "<<count<<"\n";
	int* arrivalTimeArr = new int[count];
	ConfigureTimeArrivalArray(arrivalTimeArr,& index);
	cout<<"The arrival time array is\n";
	for(int i =0;i<index;i++)
	{
		cout<<arrivalTimeArr[i]<<"\n";
	}
	std::vector< std::vector<process> > v;
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

void CountProcesses(int* count)
{
	string comment = "#";
	string line;
	ifstream in ("processes.txt");
	if (in.is_open())
	{
		cout<<"The file is opened correctly\n";
		while(getline(in,line))
		{
			cout<<"The line is " << line <<"\n";
			if(line.find(comment) == string::npos)
				(*count)++;
	    }
	}
	in.close();
}

void ConfigureTimeArrivalArray(int* arrivalTimeArr,int* index)
{
	ifstream in2 ("processes.txt");
	string arrtime ="";
	string line;
	string comment = "#";
	int flag = 0;
	if (in2.is_open())
	{
		cout<<"The file is opened correctly\n";
		while(getline(in2,line))
		{
			flag = 0;
			cout<<"The line is " << line <<"\n";
			if(line.find(comment) == string::npos)
			{
				stringstream iss(line);
				while(getline(iss,arrtime,'\t'))
				{
					if(flag == 1)
					{
						cout<<"Arrtime is "<<arrtime<<"\n";
						if(atoi(arrtime.c_str()) != arrivalTimeArr[(*index)-1])
						{
							arrivalTimeArr[*index] = atoi(arrtime.c_str());
							(*index)++;
							break;
						}
					}
					flag++;
				}
			}
		}			    
	}
	in2.close();
}
void BuildStruct(struct process *p,stringstream iss)
{
	string params="";
	p.pid = -2;
	while(getline(iss,params,'\t'))
	{
		.
	}
}