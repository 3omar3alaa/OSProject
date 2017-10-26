#include "headers.h"
#include <iostream>

void handle(int)
{
  std::cout<<"The scheduler received an alarm signal\n";
}

int main(int argc, char* argv[]) {
    
    initClk();
    
    //TODO: implement the scheduler :)
    signal(SIGALRM,handle);
    std::cout<<"The scheduler grp id is "<<getpgrp()<<"\n";
    while(1){}
    //upon termination release clock
    //destroyClk(true);
    
}
