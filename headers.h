#include <iostream> 
using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define SHKEY 300
#define MSGQKEY 11111
#define PRCMSGQKEY 22222

///==============================
//don't mess with this variable//
int* shmaddr;                  //
//===============================
struct process{
  long mtype;
  int id;
  int pid;
  int priority;
  int arrivalTime;
  int runtime;
};

struct pTime 
{
  long mtype;
  long remainingtime;
};

int getClk()
{
  
   	int clk=*shmaddr;
       return clk;		
}


/* All process call this function at the begining to establish communication
between them and the clock module 

Again, Remember that the clock is only emulation
*/
void initClk()
{
  int shmid = shmget(SHKEY, 4, 0444);
  while((int)shmid == -1)
  	{
          //Make sure that the Clock exists
        cout<<"wait, Clock not initialized yet\n"<<endl;
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
  	}

   shmaddr = (int*) shmat(shmid, (void *)0, 0);
}


/* All process call this function at the end to release the  communication
resources between them and the clock module 

Again, Remember that the clock is only emulation 

input: terminateAll : is a flag to indicate whether that 
this is the end of simulation it terminates all the system and release resources
*/

void destroyClk(bool terminateAll)
{
	cout << "CLK: Deleting shared memory" << endl;
    shmdt(shmaddr);
    //if(terminateAll)
      //killpg(getpgrp(),SIGINT);
}