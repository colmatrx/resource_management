#include<stdio.h>
#include <sys/ipc.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<strings.h>
#include<time.h>
#include<sys/wait.h>
#include<sys/msg.h>
#include<signal.h>
#include<stdlib.h>
#include "config.h"

struct msgqueue{

    long int msgtype;
    char msgcontent[15];
};

int resourceMessageID;

/*Author Idris Adeleke CS4760 Project 5 - Resource Management*/

/* Submitted December 1, 2021*/

//This is the user_proc application that gets called by the execl command inside oss child processes

int main(int argc, char *argv[]){

    struct msgqueue resourceMessageChild;    //to communicate resource request and release messages with the master

    int randomResourceID, randomResourceNum, msgsnderror, msgrcverror; 
    char rand_name[3];  //to store converted rand as string
    char resourceToken[4] = "R"; char returnString[15] = "return";
    //long int userpid = getpid(); 
    char *messageFromMaster;    //to store message content received from Master
    long int pidFromMaster;
    
    printf("\nExecuting User Process %d \n", getpid());

    resourceMessageID = msgget(message_queue_key, 0);   //get the message queue ID

    char user_rname[10]; int newrand;

    randomResourceID = randomNumber(0, 19); //generate random number between 0 and 19 for resource descriptor R'N'

    strcpy(user_rname, "");

    sleep(1); //sleep a bit to ensure a random numbere different from the first random number

    randomResourceNum = randomNumber(1, 19); // this is the number of resources that will be requested

    sprintf(rand_name, "%d", randomResourceNum); //converts randomResourceNum to string and store it in rand_name

    strcat(user_rname, rand_name);    //concatenates rand_name and name to give R0, R1 to R19

    strcat(user_rname, " ");  //append a space delimiter to user_rname

    strcat(user_rname, "R");    //append 'R' to user_rname

    sprintf(rand_name, "%d", randomResourceID); //converts randomResourceID to string

    strcat(user_rname, rand_name);    //append randomResourceID to user_rname to construct exmaple '3 R12'

    strcpy(resourceMessageChild.msgcontent, user_rname);    //copies user_rname into message content

    resourceMessageChild.msgtype = getpid();    //initialize msgtype with user process' PID

    msgsnderror = msgsnd(resourceMessageID, &resourceMessageChild, sizeof(resourceMessageChild), 0);

    if (msgsnderror == -1){ //error checking msgsnd()

        perror("\nError: In user_proc(). msgsnd() failed!");

        exit(1);
    }

    printf("\nUser Process %d is requesting resource %s from Master\n", getpid(), user_rname);

    while(1){

        sleep(2);

        msgrcverror = msgrcv(resourceMessageID, &resourceMessageChild, sizeof(resourceMessageChild), getpid(), 0); //receive message back from master

        if (msgrcverror == -1){ //error checking msgrcverror()

            perror("\nError: In user_proc(). msgrcv() failed!");

            exit(1);
        }

        if (strcmp(resourceMessageChild.msgcontent, "-1") == 0){    //if resource not granted

            printf("\nResource not granted by Master...User Process %d exiting\n", getpid());

            exit(1);
        } 

        else if (strcmp(resourceMessageChild.msgcontent, "1") == 0)   //if resource is granted
            break;      //break out of the loop if resource i granted

        else if (strcmp(resourceMessageChild.msgcontent, user_rname) == 0){ //if you read your own message back before master could read it, reconstruct it and resend it to the queue

            resourceMessageChild.msgtype = getpid();

            strcpy(resourceMessageChild.msgcontent, user_rname);    //copies user_rname into message content

            msgsnderror = msgsnd(resourceMessageID, &resourceMessageChild, sizeof(resourceMessageChild), 0);

            if (msgsnderror == -1){ //error checking msgsnd()

                perror("\nError: In user_proc(). msgsnd() failed!");

                exit(1);
            }

            continue;
        }
    }   

    printf("\nResource %s granted by Master to Process %d\n", user_rname, getpid());

    while ((newrand = randomNumber(0,19)) != randomResourceNum){  //hold on to the resource until you generate the same number you generated to request it

        printf("\nProcess %d is still using resource %s\n", resourceMessageChild.msgtype, user_rname);

        sleep(1);
    }  

    printf("\nProcess %d releasing resource %s\n", resourceMessageChild.msgtype, user_rname);

    strcat(returnString, " "); //append a space to "return"

    strcat(returnString, user_rname);   //construct a string that looks like "return 3 R10"

    strcpy(resourceMessageChild.msgcontent, "");    //copy blank in message content to ckear it out

    strcpy(resourceMessageChild.msgcontent, returnString);  //copy returnString "return 3 R10"

    resourceMessageChild.msgtype = getpid();   //copy user process pid into message type

    msgsnderror = msgsnd(resourceMessageID, &resourceMessageChild, sizeof(resourceMessageChild), 0);    //send message to master informing it of user process returning resource

    if (msgsnderror == -1){ //error checking msgsnd()

        perror("\nError: In user_proc(). msgsnd() failed!");

        exit(1);
    }

    printf("\nProcess %d completed user process execution\n", getpid());

    return 0;
}