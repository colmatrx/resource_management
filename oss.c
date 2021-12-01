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

/*Author Idris Adeleke CS4760 Project 5 - Resource Management*/

/* Submitted December 1, 2021*/

//This is the main oss application that randomly creates child processes which in turn exec user_proc application

typedef struct resources{
    
    char resourceName[4]; //resourceName = R1, R2....R20
    int totalResource; //total resource in system
    int allocatedResource;   //currently allocated resource
    int availableResource;  //currently available resource
    long int processIndex[max_number_of_processes];   //to track which process is allocated a resource; maximum of 18 processes
    int processAllocation[max_number_of_processes];  //to track how many resource instances a process is allocated

} resource; //try an integer array int processTracker[18] to track which process has what resources

struct msgqueue{

    long int msgtype;
    char msgcontent[15];
};

int resourceMessageQueueID;  //message queue ID

resource *r_descriptorAddress;  //struct pointer to traverse the shared memory

int processID[max_number_of_processes];

void initclock(void);   //function to initialize the two clocks in shared memory

void initResourceTable(void);   //function to initialize and instantiate resources R0 to R19

void printResourceAvailabilityTable(void);  //to print out the number of available resources

void printResourceAllocationTable(void);    //to print out the resource allocation table

void cleanUp(void);

void siginthandler(int);    //handles Ctrl+C interrupt

void timeouthandler(int sig);   //timeout handler function declaration

unsigned int ossclockid, *ossclockaddress, *osstimeseconds, *osstimenanoseconds; //used for ossclock shared memory

unsigned int resourceTableID;  //used for initializing resource Table Structure

unsigned int ossofflinesecondclock; unsigned int ossofflinenanosecondclock; //used as offline clock before detaching from the oss clock shared memory

char *logfilename = "logfile.log"; char logstring[4096]; 

int pid; int randomTime;

int resourceDenied = 0; int resourceReleased = 0; //to track number of resources denied and returned

int main(int argc, char *argv[]){   //start of main() function

    int msgrcverr, msgsnderr; char resourceRequest[15]; char resourceReturnCopy[15]; 

    char *resourceNum, *resourceID; char *firstToken, *secondToken, *thirdToken;

    long int mtype;

    signal(SIGINT, siginthandler); //handles Ctrl+C signal inside OSS only     

    signal(SIGALRM, timeouthandler); //handles the timeout signal

    alarm(oss_run_timeout); //fires timeout alarm after oss_run_timeout seconds defined in the config.h file

    struct msgqueue resourceMessage;    //to communicate resource request and release messages with the user process

    printf("\nMaster Process ID is %d\n", getpid());
    
    //this block initializes the ossclock

    ossclockid = shmget(ossclockkey, 8, IPC_CREAT|0766); //getting the oss clock shared memory clock before initializing it so that the id can become available to the child processes

    if (ossclockid == -1){  //checking if shared memory id was successfully created

        perror("\noss: Error: In oss main function, ossclockid cannot be created; shmget() call failed");
        exit(1);
    }

    initclock(); //calls function to initialize the oss seconds and nanoseconds clocks

    printf("\nMaster clock initialized at %hu:%hu\n", *osstimeseconds, *osstimenanoseconds); //print out content of ossclock after initialization

    //setting up message queue here

    resourceMessageQueueID = msgget(message_queue_key, IPC_CREAT|0766); //creates the message queue and gets the queue id

    printf("\nMessage Queue ID is %d\n", resourceMessageQueueID);

    if (resourceMessageQueueID == -1){  //error checking the message queue creation

        perror("\nError: Master in Parent Process. Message queue creation failed\n");

        exit(1);
    }

    //end of message queue setup


    resourceTableID = shmget(resourceTableKey, (sizeof(resource)*20), IPC_CREAT|0766); //create a shared memory allocation for 20 resources

    if (resourceTableID == -1){  //checking if shared memory id was successfully created

        perror("\noss: Error: In oss main function, resourceTableID cannot be created; shmget() call failed");
        exit(1);
    }
       
    initResourceTable(); //calls function to initialize the resource table data structure

    printf("\nMaster Resource Desctiptor Table was successfully initialized. Size of Table is %d bytes\n\n", (sizeof(resource)*20)); //print out content of ossclock after initialization
   
    for (int count = 0; count < max_number_of_processes; count++){       //randomly create user processes in this block

        pid = fork();

        if (pid < 0){
                perror("\nError: Master in main() function. fork() failed!");
                exit(1);
            }

        if (pid > 0){

            processID[count] = pid;
        }

        if (pid == 0){  //child process was created

            printf("\nUser Process %d was created\n", getpid());

            execl("./user_proc", "./user_proc", NULL);      //execute user process
        }

        randomTime = randomNumber(1,10);    //generate random number between 1 and 10

        *osstimenanoseconds+=randomTime;    //increment oss nanosecond by randomTime

        printf("\nMaster created User Process %d at %hu:%hu\n", processID[count], *osstimeseconds, *osstimenanoseconds);
    }
    
    

    while (1){

        printf("\nMaster waiting for resource request or release\n");

        mtype = 0;

        snprintf(logstring, sizeof(logstring), "\nMaster listening for resource request/release at %hu:%hu\n", *osstimeseconds+=1, *osstimenanoseconds);

        logmsg(logfilename, logstring); //calling logmsg() to write to file

        msgrcverr = msgrcv(resourceMessageQueueID, &resourceMessage, sizeof(resourceMessage), 0, 0); 

        if (msgrcverr == -1){ //error checking msgrcverror()

            perror("\nMaster in oss main() function. msgrcv() failed!");

            break;
        }

        printf("\nMaster received signal from message queue\n");

        mtype = resourceMessage.msgtype; //store message type received above

        strcpy(resourceRequest, resourceMessage.msgcontent);

        printf("Inside Master, message type is %d and message content is %s\n", mtype, resourceRequest);
        
        firstToken = strtok(resourceRequest, " ");

        if (strcmp(firstToken, "return") == 0){    //if first resource keyword is "return"

            secondToken = strtok(NULL, " ");    //secondToken is number of resources returned

            thirdToken = strtok(NULL, " ");     //thirdToken is the name of the resource returned like R2

            strcat(resourceReturnCopy, ""); strcat(resourceReturnCopy, secondToken); strcat(resourceReturnCopy, " "); strcat(resourceReturnCopy, thirdToken); 

            printf("\nProcess %d returned resource %s\n", mtype, resourceReturnCopy);

            for (int i = 0; i < 20; i++){

                if (strcmp(r_descriptorAddress[i].resourceName, thirdToken) == 0){ //search for the resource in shared memory by resource name and update the resource data structure

                    int numberOfResource = strtol(secondToken, NULL, 10);   //convert secondToken to int

                    r_descriptorAddress[i].allocatedResource -=  numberOfResource;  //subtract secondToken

                    r_descriptorAddress[i].availableResource += numberOfResource;   //add secondToken

                    for (int index = 0;  index < max_number_of_processes; index++){  //traverse the process array for the particular resource to find and remove the returning process from allocation
                            
                            if (r_descriptorAddress[i].processIndex[index] == mtype){   //find the process returning the resource
                                
                                r_descriptorAddress[i].processAllocation[index] = 0;    //reset its allocation to 0

                                r_descriptorAddress[i].processIndex[index] = 0;         //reset the array location to 0

                                break;
                            }

                            else{
                                
                                continue;
                            }

                    }

                    printf("\nAvailable %s is now %d\n", thirdToken, r_descriptorAddress[i].availableResource);

                    break;  //break out of the for loop if the resource is already found

                }

            }

            resourceReleased++; //count the number of returns

            snprintf(logstring, sizeof(logstring), "\nMaster Acknowledged Process %d Released %s %s at %hu:%hu\n", mtype, secondToken, thirdToken, *osstimeseconds+=1, *osstimenanoseconds);

            logmsg(logfilename, logstring); //calling logmsg() to write to file

            printResourceAllocationTable(); //display allocation table on release of resource

            printResourceAvailabilityTable();   //display available resources

        }

        else{

            int numOfResources = strtol(firstToken, NULL, 10); //convert firstToken to int

            secondToken = strtok(NULL, " "); //secondToken is the resource name here and firstToken will contain the number of resource

            snprintf(logstring, sizeof(logstring), "\nMaster has detected Process %d requesting %d %s at %hu:%hu\n", mtype, numOfResources, secondToken, *osstimeseconds, *osstimenanoseconds+=5);

            logmsg(logfilename, logstring); //calling logmsg() to write to file

            snprintf(logstring, sizeof(logstring), "\nMaster running deadlock avoidance algorithm at %hu:%hu\n",*osstimeseconds, *osstimenanoseconds+=5);

            logmsg(logfilename, logstring); //calling logmsg() to write to file

            printf("\nMaster received request for resource %d %s from Process %d\n", numOfResources, secondToken, mtype);  

            for (int i = 0; i < 20; i++){

                if (strcmp(r_descriptorAddress[i].resourceName, secondToken) == 0){ //search for the resource in shared memory by resource name

                    printf("\nResource %s found.\n", secondToken);

                    if (r_descriptorAddress[i].availableResource >= numOfResources){     //if resource is available, update the Resource data structure    

                        printf("\nnumber of resources requested is %d\n", numOfResources);

                        r_descriptorAddress[i].allocatedResource+=numOfResources;    //increment allocatedResource by number requested

                        r_descriptorAddress[i].availableResource = (r_descriptorAddress[i].totalResource - r_descriptorAddress[i].allocatedResource);    //decrement available resource 

                        for (int index = 0;  index < max_number_of_processes; index++){  //traverse the process array for the particular resource
                            
                            if (r_descriptorAddress[i].processIndex[index] > 0)
                                continue;       //skip the location already with a process ID in it

                            else{
                                
                                r_descriptorAddress[i].processIndex[index] = mtype; //store the requesting process' ID in the process array of the resource being requested

                                r_descriptorAddress[i].processAllocation[index] = numOfResources; //store the corresponding number of resource requested

                                break;  //break out of the loop once the process ID has been stored

                            }

                        }
                        
                        strcpy(resourceMessage.msgcontent, "1"); //send 1 to user process to indicate resource was granted
                        resourceMessage.msgtype = mtype;

                        msgsnderr = msgsnd(resourceMessageQueueID, &resourceMessage, sizeof(resourceMessage), IPC_NOWAIT);   //send message granted to user process

                        sleep(2);

                        printf("\nResource available; Master sent message type %d and message content %s to user process\n", mtype, "1");

                        snprintf(logstring, sizeof(logstring), "\nMaster granted %d %s to Process %d at %hu:%hu\n\n", numOfResources, secondToken, mtype, *osstimeseconds, *osstimenanoseconds);

                        logmsg(logfilename, logstring); //calling logmsg() to write to file

                        printResourceAllocationTable(); //log allocation table on granting of resource

                        printResourceAvailabilityTable();
                    }

                    else{   //terminate user process if enough resource is not available to avoid deadlock

                            printf("\nMaster denying user process %d resource %s for insufficient availability of resource\n", mtype, secondToken);

                            resourceMessage.msgtype = mtype;

                            strcpy(resourceMessage.msgcontent, "-1");   //send user process -1 if resource is not available

                            msgsnderr = msgsnd(resourceMessageQueueID, &resourceMessage, sizeof(resourceMessage), IPC_NOWAIT);

                            printf("\nResource unavailable; Master sent message type %d and message content %s to user process\n", mtype, "-1");

                            sleep(3);

                            printf("\nMaster terminating user process %d to avoid deadlock\n", mtype);

                            kill(mtype, SIGKILL);

                            resourceDenied++;   //count the number of resource denials

                            snprintf(logstring, sizeof(logstring), "\nMaster denied %d %s to Process %d at %hu:%hu to avoid deadlock\n", numOfResources, secondToken, mtype, *osstimeseconds, *osstimenanoseconds);

                            logmsg(logfilename, logstring); //calling logmsg() to write to file

                            printResourceAvailabilityTable();

                            break;

                    }

                }


            }
        }

        sleep(3);

        if (max_number_of_processes == (resourceDenied + resourceReleased)) //if all processes have either been denied or have released their resources, break out of listening for request
            break;
    }

    cleanUp();      //call cleanup before exiting main() to free up used resources

    snprintf(logstring, sizeof(logstring), "\nMaster: No more requests. Master completed execution at %hu:%hu\n", ossofflinesecondclock+=1, ossofflinenanosecondclock+=05);

    logmsg(logfilename, logstring); //calling logmsg() to write to file

    return 0;

}   //end of main function


//initclock() function initializes the ossclock 

void initclock(){ //initializes the seconds and nanoseconds parts of the oss

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
    
    int randomNumberResult;

    r_descriptorAddress  = shmat(resourceTableID, NULL, 0); //shmat returns the address of the shared memory, receive the address into a structure pointer variable

    if (r_descriptorAddress == (void *) -1){

        perror("\nMaster: Error: In initResourceTable(), r_descriptorAddress not returned by shmat()");
        exit(1);

    }

    snprintf(logstring, sizeof(logstring), "\nMaster Initializing Resource Descriptor Table at %hu:%hu\n\n", *osstimeseconds, *osstimenanoseconds+=55);

    logmsg(logfilename, logstring); //calling logmsg() to write to file

    //loop through the resource table in shared memory using structure address pointer to initialize the resource arrtibutes

    for (int i = 0; i < 20; i++){

        char rname[4] = "R";    //using 4 characters because it stores 3 characters and a NULL

        sprintf(name, "%d", i); //converts i to string

        strcat(rname, name);    //concatenates rname and name to give R0, R1 to R19

        strcpy(r_descriptorAddress[i].resourceName, rname); //copies rname to resourceName

        randomNumberResult = randomNumber(1, 10);  //generate a number between 1 and 90 to initialize the total instances of resource R[i]

        sleep(1);

        r_descriptorAddress[i].totalResource = randomNumberResult; //initalize resources with random values

        r_descriptorAddress[i].allocatedResource = 0;

        r_descriptorAddress[i].availableResource = r_descriptorAddress[i].totalResource;    //available resource = total resource at the initial state

    }

    //print out resource table structure with total resources after initialization

    printf("\nResource Table after Initialization\n\n");

   for (int i = 0; i <= 20; i++){       //loop to print the resource names as headers

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


    for (int i = 0; i < 20; i++){       //loop to print the corresponding values under each resource header

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

    snprintf(logstring, sizeof(logstring), "\n\nMaster completed Resource Descriptor Initialization at %hu:%hu\n", *osstimeseconds+=2, *osstimenanoseconds+=30);
    
    logmsg(logfilename, logstring);

}   //end of initResourceTable()


void printResourceAvailabilityTable(void){  //prints the number of available resources when called

    strcpy(logstring, "");

    snprintf(logstring, sizeof(logstring), "\nMaster Updating Resource Availability Table at %hu:%hu\n\n", *osstimeseconds, *osstimenanoseconds+=55);

    logmsg(logfilename, logstring); //calling logmsg() to write to file


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
        sprintf(resourceToString, "%d", r_descriptorAddress[i].availableResource); //converts int to string

        if (i == 0){
            
            strcat(logstring, resourceToString);
            printf("%d \t", r_descriptorAddress[i].availableResource);
            continue;
       }
       
        strcat(logstring, "\t");
        strcat(logstring, resourceToString);
        printf("%d \t", r_descriptorAddress[i].availableResource);

    }

    logmsg(logfilename, logstring);

    snprintf(logstring, sizeof(logstring), "\n\nMaster Updated Resource Availability Table at %hu:%hu\n", *osstimeseconds, *osstimenanoseconds+=30);
    
    logmsg(logfilename, logstring);

}   //end of printResourceAvailabilityTable()


void printResourceAllocationTable(void){    //function to print the resource allocation table

    strcpy(logstring, "");

    snprintf(logstring, sizeof(logstring), "\nMaster Updating Resource Allocation Table at %hu:%hu\n\n", *osstimeseconds, *osstimenanoseconds+=55);

    logmsg(logfilename, logstring); //calling logmsg() to write to file

    printf("%s", logstring);

    char indexToString[6]; char allocationToString[4];  //for conversions to strings

    for (int i = 0; i < 20; i++){    
        
        strcat(logstring, r_descriptorAddress[i].resourceName);
        strcat(logstring, " ->");

        for (int index = 0; index < max_number_of_processes; index++){

                if (r_descriptorAddress[i].processIndex[index] > 0){    //if there a process ID in this location

                    sprintf(indexToString, "%d", r_descriptorAddress[i].processIndex[index]); //converts int to string

                    sprintf(allocationToString, "%d", r_descriptorAddress[i].processAllocation[index]); //converts int to string

                    strcat(logstring, "\t"); strcat(logstring, "PID "); strcat(logstring, indexToString);

                    strcat(logstring, "("); strcat(logstring, allocationToString); strcat(logstring, ")"); //to form example R0 -> PID 1445(5)

                }
        }

        strcat(logstring, "\n");
    }

    logmsg(logfilename, logstring);

    printf("%s", logstring);

    snprintf(logstring, sizeof(logstring), "\nMaster Updated Resource Allocation Table at %hu:%hu\n", *osstimeseconds, *osstimenanoseconds+=30);
    
    logmsg(logfilename, logstring);

    printf("%s", logstring);

}   //end of printResourceAllocationTable()


void cleanUp(void){ //frees up used resources including shared memory

    char pidToString[6];

    printf("\nCleaning up used resources....\n");

    snprintf(logstring, sizeof(logstring), "\nMaster cleaning up resources at %hu:%hu\n", *osstimeseconds, *osstimenanoseconds+=5);
    logmsg(logfilename, logstring);

    // first kill all user processes that are still holding resources

    snprintf(logstring, sizeof(logstring), "\nMaster stopping all pending user processes at %hu:%hu\n", resourceMessageQueueID, ossofflinesecondclock, ossofflinenanosecondclock+=15);
    logmsg(logfilename, logstring);

        for (int index = 0; index < max_number_of_processes; index++){

                if (processID[index] > 0){    //if there a process ID in this location

                    sprintf(pidToString, "%d", processID[index]);   //convert process id to string

                    printf("\nStopping user process %d\n", processID[index]);

                    snprintf(logstring, sizeof(logstring), "\nMaster stopping Process %s at %hu:%hu\n", pidToString, ossofflinesecondclock, ossofflinenanosecondclock+=15);
                    logmsg(logfilename, logstring);

                    kill(processID[index], SIGKILL);
                }
        }

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

    snprintf(logstring, sizeof(logstring), "\nMaster Resource Descriptor Table's Shared Memory ID %hu has been detached and deleted at  %hu:%hu\n", resourceTableID, *osstimeseconds+=1, *osstimenanoseconds+=15);
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

    if ( msgctl(resourceMessageQueueID, IPC_RMID, 0) == 0)
        printf("\nMessage Queue ID %d has been removed.\n", resourceMessageQueueID);

    else{    
        printf("\nErrror: Master in cleanUp(), Message Queue removal failed!\n");
        exit(1);
    }

    snprintf(logstring, sizeof(logstring), "\nMaster removed Messaqe Queue ID %d at %hu:%hu\n", resourceMessageQueueID, ossofflinesecondclock, ossofflinenanosecondclock+=15);
    logmsg(logfilename, logstring);


}   //end of cleanUP()


void siginthandler(int sigint){

        printf("\nMaster: Ctrl+C interrupt received. In siginthandler() handler. Aborting Processes..\n");

        snprintf(logstring, sizeof(logstring), "\nMaster in Signal Handler; Ctrl+C interrupt received at %hu:%hu\n", *osstimeseconds+=1, *osstimenanoseconds);
        logmsg(logfilename, logstring);

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

    cleanUp(); //call cleanup to free up used resources
    snprintf(logstring, sizeof(logstring), "\nMaster terminating at %hu:%hu", ossofflinesecondclock+=1, ossofflinenanosecondclock+=5);
    logmsg(logfilename, logstring);

    kill(getpid(), SIGTERM);

    exit(1);

}   //end of timeouthandler()