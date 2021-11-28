#include<stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<strings.h>
#include<time.h>
#include<sys/wait.h>
#include<signal.h>
#include<stdlib.h>

/*Author Idris Adeleke CS4760 Project 2 - Concurrent Linux Programming and SHared Memory*/
//This is the testsim application that gets called by the execl command inside runsim

/*void testsim(int, int); //function declaration

int shared_memoryid; 
char *shared_memoryaddress;
int shared_memory_key = 32720;*/



int main(int argc, char *argv[]){

   /* long int arg1, arg2;

    arg1 = strtol(argv[1], NULL, 10); arg2 = strtol(argv[2], NULL, 10); //using strtol() to convert the string arg argv[2] to integer

    printf("\nexecuting testsim\n");

    /*if ((shared_memoryid = shmget(shared_memory_key, 100, 0666)) == -1){   //call to create a shared memory segment with shmget() with permissions 0666
        perror("\nrunsim: Error: In testsim application: shmget call failed."); //error checking shmget() call
        exit(1);
    }

    if ((shared_memoryaddress = shmat(shared_memoryid, NULL, 0)) == (char *) -1){  //call to shmat() to return the memory address of the shared_memory_id
        perror("\nrunsim: Error: In testsim application: shmat call failed.\n");  //error checking shmat() call
        exit(1);
    }*/

   // testsim(arg1, arg2 ); //takes command line args sleeptime and repeatfactor as integers

   /* if ((shmdt(shared_memoryaddress)) == -1){       //call to shmdt() to detach from the shared memory address
        perror("\nrunsim: Error: In testsim application: Shared memory cannot be detached\n");
        exit(1);
    }*/

    /*printf("\nProcess %d completed execution\n", getpid());

    printf("\nShared memory was successfully detached from by Child process %d\n", getpid());*/

    return 0;

}