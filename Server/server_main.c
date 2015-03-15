/* 
 * File:   main.c
 * Author: maziar
 *
 * Created on December 6, 2009, 1:08 PM
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <math.h>
#include <bits/waitstatus.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#define SHMSZ   1024
#define MAX_LINE_LENGHT 200
#define MAX_CLIENTS 10
#define PORT_NO 99997
#define SHM_KEY 4567


typedef struct spid
{
    int id,sock;
}spid;

char *shm;
key_t key = SHM_KEY;         //name of the key that client must be know
int client_no = 0;
pthread_mutex_t shm_write_lock;
pthread_mutex_t log_write_lock;
pthread_mutex_t piping_lock;
pthread_t cleint_thread[MAX_CLIENTS];
int clientpid[MAX_CLIENTS];
int exit_command = 0;
void Shminit();
void* commander();
void command_writer();
void* connector(void*);
void* newclient(void*);

int main(int argc, char** argv)
{
    srand(time(NULL));
    pthread_t thread1, thread2, thread3;
    int  iret1, iret2, iret3, logfd;
    logfd = fopen("log.txt","w");
    close(logfd);

    iret1 = pthread_create( &thread1, NULL, &commander, NULL);      //in this thread shared memory has initialized and command getter while is here
    iret2 = pthread_create( &thread2, NULL, &connector, NULL);      //this thread accept new clients and run their handlers concurrent

    pthread_join( thread1, NULL);
    if(exit_command)
        return 0;

    pthread_join( thread2, NULL);

    return (EXIT_SUCCESS);
}


/*
 * connector function body
 */
 void* connector(void* bimored)
{
    int sockfd, new_fd,i; // listen on sock_fd, new connection on new_fd
    struct spid temp;
    int temp_port = 0;
	struct sockaddr_in my_addr; // my address information struct
	struct sockaddr_in their_addr; // connector s address information int sin_size;
	char buffer[256];

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) // do error checking!
	{
		printf("systom fosh dad...\n");
        exit(0);
	}

	my_addr.sin_family = AF_INET; // host byte order
	my_addr.sin_port = htons(PORT_NO); // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // autofill with my IP
	memset(&(my_addr.sin_zero), '\0' , 8); // zero the rest of the struct
	bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	listen(sockfd, MAX_CLIENTS);    //listen for all of client
	socklen_t sin_size = (socklen_t)sizeof(struct sockaddr_in);
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        //in this step server is waiting for new client
        if ( (new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) < 0)  //wait for a client
        {
            printf("accept failed\n");      //error in client accaptation
            continue;
        }
        else
        {
            //printf("new client accepted...\n");

            int  error_code;

            handshaker(new_fd);     // client and server hand shaking function
            temp.id = clientpid[client_no];     // set client pID for signaling
            temp.sock = new_fd;
            //create new client manager thread... :D
            error_code = pthread_create( &cleint_thread[client_no], NULL, &newclient, (void*)&temp);
            if(error_code != 0)
            {
                printf("we have some errors with thread creating, error code %d\n", error_code);
            }
            else
            {       //new client connect successfully so clients number is add by one
                client_no++;
            }
            //inja bayad ba voroodi e socket yek thread baz beshe
            //bad ham handshaking beshe va payam e client chaap beshe
            //pid un ham bayad too list e pid haa zakhire beshe
            //baraye mogheE ke mikhaym baraye hamashoon signal befrestim
        }
    }
    
    //ye fekri ham bayad baraye join e thread haE ke misazim bokonim
}

 void* newclient(void* tempspid)
{
    struct spid tempp = *((spid*) tempspid);
    int sockfd,logfd,fd[2],n;
    char* rec_command = (char*) malloc(256*sizeof(char));
    char buffer[201];

    sockfd = tempp.sock;

    while(1)
    {

        bzero(rec_command,256);
        read(sockfd,rec_command,256);   //get client's command
        

        pthread_mutex_lock(&log_write_lock);    //write log in a file
        logfd = fopen("log.txt","a");
        fprintf(logfd,"client number %d command : %s\n", tempp.id, rec_command);
        fclose(logfd);

        pthread_mutex_unlock(&log_write_lock);

        if(strncmp(rec_command,"exit",4) == 0)      //perform exit on exit command
        {
            close(sockfd);
            free(rec_command);
            exit_command = 1;
            pthread_exit(0);
            break;
        }
        else
        {
            pthread_mutex_lock(&piping_lock);

            // here should command run by server and its output write to socket
            int stout = dup(1),errorrr = dup(2);    //save old std IO to restor after piping

            pipe(fd);       //piping
            dup2(fd[1],1);  //stdout
            dup2(fd[1],2);  //error

            system(rec_command);    //run client's command

            n = 200;
            while(n == 200)         //write the out put on client-server connector socket
            {
                bzero(buffer,201);
                n = read(fd[0],buffer,200);
                write(sockfd,buffer,n);
            }
            dup2(stout,1);      //restore fd s
            dup2(errorrr,2);    //restore fd s
            close(fd[0]);       //restore fd s
            close(fd[1]);       //restore fd s
            pthread_mutex_unlock(&piping_lock);
        }
    }

    printf("exiting time\n");
    free(rec_command);
}

 int handshaker(int new_fd)         //hand shaking function
 {
     char* temp;
     temp = (char*) malloc(256*sizeof(char));
     char buffer[256];
     bzero(buffer,256);
     read(new_fd,buffer,256);   //read pid
     clientpid[client_no] = atoi(buffer);
     
     bzero(buffer,256);         //read client's comming message
     read(new_fd,buffer,256);

     sprintf(temp,"%d",SHM_KEY);    //send shared memory key to client
     write(new_fd, temp, strlen(temp));
 }

void* commander()
{
    Shminit();      //berahing share memory
    int lent,i;
    printf("\n");
    char* command = (char*)malloc(sizeof(char) * MAX_LINE_LENGHT);

    while(1)        //main while of server (user work with it)
    {
        printf("baziar@server : ");
        if(getline(&command,&lent,stdin) == EOF)    //ctrl+D
			break;
        if(strncmp(command,"exit",4) == 0)         //exit command
        {
            printf("server exited now...!\n");
            break;
        }
        else if(strncmp(command,"log",3) == 0)          //viewing log
        {
            printf("log is here!\n---------------------------------\n\n");
            system("cat log.txt");
        }
        else
        {
            //printf("send 2 All client >> write to share memory then send signals to client\n\n");
            command_writer(command);

            for(i = 0; i < client_no; i++)  //send signal to all client to read server message from shared memory
                kill(clientpid[i],SIGUSR1);

            //now we want to broadcast a signal around the system

        }
    }
    free(command);
}

void command_writer(char* command)      //write a command (string) to shared memory
{
    pthread_mutex_lock(&shm_write_lock);
    strcpy(shm,command);
//    printf("shared memory contains : %s\n",shm);
    pthread_mutex_unlock(&shm_write_lock);
}

void Shminit()
{
    char c;
    int shmid;

    //create segment

    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    //attach segment to server's data space
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

}
