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

//resource r_decriptor[20]; //array of resource descriptors


void initclock(void);   //function to initialize the two clocks in shared memory

void initResourceTable(void);   //function to initialize and instantiate resources R0 to R19

int randomNumber(int , int ); //fucntion to generate random nunbers

unsigned int ossclockid, *ossclockaddress, *osstimeseconds, *osstimenanoseconds; //used for ossclock shared memory

unsigned int resourceTableID;  //used for initializing resource Table Structure

char *logfilename = "logfile.log"; char logstring[4096];

int main(int argc, char *argv[]){

//this block initializes the ossclock

    ossclockid = shmget(ossclockkey, 8, IPC_CREAT|0766); //getting the oss clock shared memory clock before initializing it so that the id can become available to the child processes

    if (ossclockid == -1){  //checking if shared memory id was successfully created

        perror("\noss: Error: In oss main function, ossclockid cannot be created; shmget() call failed");
        exit(1);
    }

    initclock(); //calls function to initialize the oss seconds and nanoseconds clocks

    printf("\noss seconds time is %d and nanseconds time is %d\n", *osstimeseconds, *osstimenanoseconds); //print out content of ossclock after initialization

//this block initializes the resource table for 20 resources

resourceTableID = shmget(resourceTableKey, (sizeof(resource)*20), IPC_CREAT|0766); //create a shared memory allocation for 20 resources

    if (resourceTableID == -1){  //checking if shared memory id was successfully created

        perror("\noss: Error: In oss main function, resourceTableID cannot be created; shmget() call failed");
        exit(1);
    }

    initResourceTable(); //calls function to initialize the resource table data structure

    printf("\n\nResource Table was successfully initialized. Size of Table is %d bytes\n\n", (sizeof(resource)*20)); //print out content of ossclock after initialization


return 0;

}   //end of main function


//initclock() function initializes the ossclock 

void initclock(void){ //initializes the seconds and nanoseconds parts of the oss

    ossclockaddress = shmat(ossclockid, NULL, 0); //shmat returns the address of the shared memory
    if (ossclockaddress == (void *) -1){

        perror("\noss: Error: In initclock(), ossclockaddress not returned by shmat()");
        exit(1);

    }

    osstimeseconds = ossclockaddress + 0;   //the first 4 bytes of the address stores the seconds part of the oss clock, note the total address space is for 8 bytes from shmget above
    osstimenanoseconds = ossclockaddress + 1;   //the second 4 bytes of the address stores the seconds part of the oss clock

    *osstimeseconds = 0;    //storing integer data in the seconds part of the oss clock
    *osstimenanoseconds = 0;    //storing int data in the nanoseconds part of the oss clock

}

//initResourceTable() function initializes the resource Table for 20 resources R1...R20 

void initResourceTable(void){

    char name[3]; 
    resource *r_descriptorAddress;
    int randomNumberResult;

    r_descriptorAddress  = shmat(resourceTableID, NULL, 0); //shmat returns the address of the shared memory, receive the address into a structure pointer variable

    if (r_descriptorAddress == (void *) -1){

        perror("\noss: Error: In initResourceTable(), r_descriptorAddress not returned by shmat()");
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

        printf("\nr_descriptor[%d] name = %s\n", i, r_descriptorAddress[i].resourceName);

        randomNumberResult = randomNumber(1, (100-i));  //initalize resources with random values

        //printf("\nrandomNumber returns %d\n", randomNumberResult);

        r_descriptorAddress[i].totalResource = randomNumberResult; //generate a number between 1 and 90 to initialize the total instances of resource R[i]

        r_descriptorAddress[i].allocatedResource = 0;

        r_descriptorAddress[i].availableResource = 0;

        //printf("\nr_descriptor[%d] name = %s", i, r_descriptorAddress[i].resourceName);

        //printf("\nr_descriptor[%d] total resources = %d\n", i, r_descriptorAddress[i].totalResource);

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
}

int randomNumber(int lowertimelimit, int uppertimelimit){

    int randNum = 0;
    srand(time(NULL));          //initilize the rand function
    randNum = (rand() % ((uppertimelimit - lowertimelimit) + 1)) + lowertimelimit; //this logic produces a number between lowertimelit and uppertimelimit
    return randNum;

}