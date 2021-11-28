#include<stdio.h>
#include<stdlib.h>
#include <sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<strings.h>
#include<time.h>
#include<sys/wait.h>
#include<signal.h>
#include<sys/msg.h>
#include<getopt.h>
#include<sys/sem.h>
#include "config.h"

typedef struct resources{
    
    char resourceName[4]; //resourceName = R1, R2....R20
    int totalResource; //total resource in system
    int allocatedResource;   //currently allocated resource
    int availableResource;  //currently available resource

} resource;

resource *r_descriptorAddress;  //struct pointer to traverse the shared memory

void initclock(void);   //function to initialize the two clocks in shared memory

void initResourceTable(void);   //function to initialize and instantiate resources R0 to R19

int randomNumber(int , int ); //fucntion to generate random nunbers

void cleanUp(void);

void siginthandler(int);    //handles Ctrl+C interrupt

void timeouthandler(int sig);   //timeout handler function declaration

unsigned int ossclockid, *ossclockaddress, *osstimeseconds, *osstimenanoseconds; //used for ossclock shared memory

unsigned int resourceTableID;  //used for initializing resource Table Structure

unsigned int ossofflinesecondclock; unsigned int ossofflinenanosecondclock; //used as offline clock before detaching from the oss clock shared memory

char *logfilename = "logfile.log"; char logstring[4096];



int main(int argc, char *argv[]){   //start of main() function

    signal(SIGINT, siginthandler); //handles Ctrl+C signal inside OSS only     

    signal(SIGALRM, timeouthandler); //handles the timeout signal

    alarm(oss_run_timeout); //fires timeout alarm after oss_run_timeout seconds defined in the config.h file
    
    //this block initializes the ossclock

    ossclockid = shmget(ossclockkey, 8, IPC_CREAT|0766); //getting the oss clock shared memory clock before initializing it so that the id can become available to the child processes

    if (ossclockid == -1){  //checking if shared memory id was successfully created

        perror("\noss: Error: In oss main function, ossclockid cannot be created; shmget() call failed");
        exit(1);
    }

    initclock(); //calls function to initialize the oss seconds and nanoseconds clocks

    printf("\nMaster clock initialized at %hu:%hu\n", *osstimeseconds, *osstimenanoseconds); //print out content of ossclock after initialization

    resourceTableID = shmget(resourceTableKey, (sizeof(resource)*20), IPC_CREAT|0766); //create a shared memory allocation for 20 resources

    if (resourceTableID == -1){  //checking if shared memory id was successfully created

        perror("\noss: Error: In oss main function, resourceTableID cannot be created; shmget() call failed");
        exit(1);
    }
       
    initResourceTable(); //calls function to initialize the resource table data structure

    printf("\n\nMaster Resource Desctiptor Table was successfully initialized. Size of Table is %d bytes\n\n", (sizeof(resource)*20)); //print out content of ossclock after initialization
    while (1);
    cleanUp();      //call cleanup before exiting main() to free up used resources

    return 0;

}   //end of main function


//initclock() function initializes the ossclock 

void initclock(void){ //initializes the seconds and nanoseconds parts of the oss

    ossclockaddress = shmat(ossclockid, NULL, 0); //shmat returns the address of the shared memory
    if (ossclockaddress == (void *) -1){

        perror("\nMaster in initclock(), ossclockaddress not returned by shmat()");
        exit(1);

    }

    osstimeseconds = ossclockaddress + 0;   //the first 4 bytes of the address stores the seconds part of the oss clock, note the total address space is for 8 bytes from shmget above
    osstimenanoseconds = ossclockaddress + 1;   //the second 4 bytes of the address stores the seconds part of the oss clock

    *osstimeseconds = 0;    //storing integer data in the seconds part of the oss clock
    *osstimenanoseconds = 0;    //storing int data in the nanoseconds part of the oss clock

}   //end of initclock()


//initResourceTable() function initializes the resource Table for 20 resources R1...R20 

void initResourceTable(void){

    char name[3]; 
    *r_descriptorAddress;  //struct pointer to traverse the shared memory
    int randomNumberResult;

    r_descriptorAddress  = shmat(resourceTableID, NULL, 0); //shmat returns the address of the shared memory, receive the address into a structure pointer variable

    if (r_descriptorAddress == (void *) -1){

        perror("\nMaster: Error: In initResourceTable(), r_descriptorAddress not returned by shmat()");
        exit(1);

    }

    snprintf(logstring, sizeof(logstring), "\n\nMaster Initializing Resource Descriptor Table at %hu:%hu\n\n", *osstimeseconds, *osstimenanoseconds+=55);

    logmsg(logfilename, logstring); //calling logmsg() to write to file

    //loop through the resource table in shared memory using structure address pointer to initialize the resource arrtibutes

    for (int i = 0; i < 20; i++){

        char rname[4] = "R";    //using 4 characters because it stores 3 characters and a NULL

        sprintf(name, "%d", i); //converts i to string

        strcat(rname, name);    //concatenates rname and name to give R0, R1 to R19

        strcpy(r_descriptorAddress[i].resourceName, rname); //copies rname to resourceName

        randomNumberResult = randomNumber(1, (100-i));  //generate a number between 1 and 90 to initialize the total instances of resource R[i]

        r_descriptorAddress[i].totalResource = randomNumberResult; //initalize resources with random values

        r_descriptorAddress[i].allocatedResource = 0;

        r_descriptorAddress[i].availableResource = 0;

    }

    //print out resource table structure with total resources after initialization

    printf("\nResource Table after Initialization\n\n");

   for (int i = 0; i <= 20; i++){       

       if (i == 0){

            strcpy(logstring, r_descriptorAddress[i].resourceName);
            printf("%s \t", r_descriptorAddress[i].resourceName);
            continue;
       }

       if (i == 20){

           printf("\n");
           strcat(logstring, "\n\n");
           break;
       }

        strcat(logstring, "\t");
        strcat(logstring, r_descriptorAddress[i].resourceName);
        printf("%s \t", r_descriptorAddress[i].resourceName);
    }


    for (int i = 0; i < 20; i++){

        char resourceToString[4];
        sprintf(resourceToString, "%d", r_descriptorAddress[i].totalResource); //converts int to string

        if (i == 0){
            
            strcat(logstring, resourceToString);
            printf("%d \t", r_descriptorAddress[i].totalResource);
            continue;
       }
       
        strcat(logstring, "\t");
        strcat(logstring, resourceToString);
        printf("%d \t", r_descriptorAddress[i].totalResource);

    }

    logmsg(logfilename, logstring);

    snprintf(logstring, sizeof(logstring), "\n\nMaster completed Resource Descriptor Initialization at %hu:%hu", *osstimeseconds+=2, *osstimenanoseconds+=30);
    
    logmsg(logfilename, logstring);

}   //end of initResourceTable()


int randomNumber(int lowertimelimit, int uppertimelimit){

    int randNum = 0;
    srand(time(NULL));          //initilize the rand function
    randNum = (rand() % ((uppertimelimit - lowertimelimit) + 1)) + lowertimelimit; //this logic produces a number between lowertimelit and uppertimelimit
    return randNum;

}   //end of randomNumber()


void cleanUp(void){ //frees up used resources including shared memory

    printf("\nCleaning up used resources....\n");

    snprintf(logstring, sizeof(logstring), "\nMaster cleaning up resources at %hu:%hu", *osstimeseconds, *osstimenanoseconds+=5);
    logmsg(logfilename, logstring);

    //Now detach and delete shared memories

     if ((shmdt(r_descriptorAddress)) == -1){    //detaching from the process table shared memory

        perror("\nMaster: Error: In cleanUp() function, r_descriptorAddress shared memory cannot be detached");
        exit(1);
    }

    printf("\nMaster resource table shared memory was detached.\n");

    if (shmctl(resourceTableID, IPC_RMID, NULL) != 0){      //shmctl() marks the oss resource descriptor table shared memory for destruction so it can be deallocated from memory after no process is using it
        perror("\nMaster in cleanUp() function, r_descriptorAddress shared memory segment cannot be marked for destruction\n"); //error checking shmctl() call
        exit(1);
    }

    printf("\nMaster resource table shared memory ID %hu was deleted.\n\n", resourceTableID);

    snprintf(logstring, sizeof(logstring), "\n\nMaster Resource Descriptor Table's Shared Memory ID %hu has been detached and deleted at  %hu:%hu\n", resourceTableID, *osstimeseconds+=1, *osstimenanoseconds+=15);
    logmsg(logfilename, logstring);

    ossofflinesecondclock = *osstimeseconds; ossofflinenanosecondclock = *osstimenanoseconds; //save the clock before detaching from clock shared memory

    if ((shmdt(ossclockaddress)) == -1){    //detaching from the oss clock shared memory

        perror("\nMaster in cleanUp() function, OSS clock address shared memory cannot be detached");
        exit(1);
    }

    printf("\nMaster clock shared memory was detached.\n");

    if (shmctl(ossclockid, IPC_RMID, NULL) != 0){      //shmctl() marks the oss clock shared memory for destruction so it can be deallocated from memory after no process is using it
        perror("\nMaster in cleanUp() function, OSS clockid shared memory segment cannot be marked for destruction\n"); //error checking shmctl() call
        exit(1);
    }

    printf("\nMaster clock shared memory ID %hu was deleted.\n\n", ossclockid);

    ossofflinesecondclock+=1; ossofflinenanosecondclock+=25;

    snprintf(logstring, sizeof(logstring), "\nMaster Clock Shared Memory ID %hu has been detached and deleted at %hu:%hu\n", ossclockid, ossofflinesecondclock, ossofflinenanosecondclock);
    logmsg(logfilename, logstring);

}   //end of cleanUP()

void siginthandler(int sigint){

        printf("\nMaster: Ctrl+C interrupt received. In siginthandler() handler. Aborting Processes..\n");

        snprintf(logstring, sizeof(logstring), "\nMaster in Signal Handler; Ctrl+C interrupt received at %hu:%hu\n", *osstimeseconds+=1, *osstimenanoseconds);
        logmsg(logfilename, logstring);

        /*snprintf(logstring, sizeof(logstring), "\nOSS: Error!: Ctrl+C interrupt was received at time %d seconds and %d nanoseconds. Aborting User Processes...",*osstimeseconds, *osstimenanoseconds);
        logmsg(logfilename, logstring);

        for (int i = 0; i < 18; i++){       //traverse the process table to locate and kill user processes
            if (((processtableaddress+i)->processid) > 0){

                kill((processtableaddress+i)->processid, SIGKILL); //kills any process with pid greater than 0
                snprintf(logstring, sizeof(logstring), "\nOSS: User Process %d terminated", (processtableaddress+i)->processid);
                logmsg(logfilename, logstring);

            }
        }*/

        cleanUp();  //calling cleanUp() before terminating oss

        snprintf(logstring, sizeof(logstring), "\nMaster terminating at %hu:%hu\n", ossofflinesecondclock+=1, ossofflinenanosecondclock+=15);
        logmsg(logfilename, logstring);
        
        kill(getpid(), SIGTERM);

        exit(1);
}   //end of siginthandler()


void timeouthandler(int sig){   //this function is called if the program times out after oss_run_timeout seconds. Handle killing child processes and freeing resources in here later

    printf("\nMaster timed out. In timeout handler. Aborting Processes..\n");

    snprintf(logstring, sizeof(logstring), "\nMaster timed out at %hu:%hu", *osstimeseconds+=1, *osstimenanoseconds+=5);
    logmsg(logfilename, logstring);

   /* *osstimeseconds = *osstimeseconds + 180; //timeout happens after 3 minutes

    snprintf(logstring, sizeof(logstring), "\nOSS: System Timeout at %d seconds and %d nanoseconds. In Timeout Signal Handler. Aborting User Processes and Cleaning up resources...", *osstimeseconds, *osstimenanoseconds);
    logmsg(logfilename, logstring);

    for (int i = 0; i < 18; i++){       //traverse the process table to locate and kill user processes
            if (((processtableaddress+i)->processid) > 0){

                kill((processtableaddress+i)->processid, SIGKILL); //kills any process with pid greater than 0
                snprintf(logstring, sizeof(logstring), "\nOSS: User Process %d terminated", (processtableaddress+i)->processid);
                logmsg(logfilename, logstring);

            }
    }*/

   // *osstimeseconds = *osstimeseconds + 1; *osstimenanoseconds = *osstimenanoseconds + 15; //increment clock before calling cleanup()

    cleanUp(); //call cleanup to free up used resources
    snprintf(logstring, sizeof(logstring), "\nMaster terminating at %hu:%hu", ossofflinesecondclock+=1, ossofflinenanosecondclock+=5);
    logmsg(logfilename, logstring);

    kill(getpid(), SIGTERM);

    exit(1);

}   //end of timeouthandler()