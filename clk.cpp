#include "headers.h"
#include <iostream>

int shmid;
int schId;

void cleanup(int)
{
	cout << "ClK: Clearing resources ..." << endl;
	shmctl(shmid,IPC_RMID,NULL);
	raise(9);
}

int main(int argc,char* argv[]) 
{
	schId = strtol(argv[0], NULL, 10);
	cout<<"Clock Starting with pid "<<getpid()<<" and SchID "<<schId<<endl;  

	signal(SIGINT, cleanup);
	int clk=0;
 
	shmid = shmget(SHKEY, 4, IPC_CREAT|0644);
	if((long)shmid == -1)
  	{
  		cout<<"Error in create shm"<<endl;
  		exit(-1);
  	}

	int * shmaddr = (int*) shmat(shmid, (void *)0, 0);
	if((long)shmaddr == -1)
	{	
  	cout<<"Error in attach in parent"<<endl;
  	exit(-1);
	}
	else
	{	
   	*shmaddr = clk;
	}

	int ppid = getppid();
	while(1)
	{
		sleep(1);
		(*shmaddr)++;
		kill(ppid, SIGURG);
		kill(schId, SIGURG);
	}
}