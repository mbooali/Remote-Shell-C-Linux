/* 
 * File:   main.c
 * Author: maziar
 *
 * Created on December 6, 2009, 1:09 PM
 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define MAX_LINE_LENGHT 256
#define SHMSZ     1024
#define PORT_NO 99997
#define SERVER_IP "127.0.0.1"

void Shminit();
void sharememprinter();
int firstconnect();
int handshaking(int sockfd);
void command_get_send(int sockfd);
char *shm;
key_t key;  //key for shared memory



int main(int argc, char** argv) {
    int sockfd;
    signal(SIGUSR1,sharememprinter);        //register user signal with its handler to handle shared memory print signal
    sockfd = firstconnect(&sockfd);         //first connectiong process of a client
    if(sockfd == -1)
        return (EXIT_FAILURE);

    if(handshaking(sockfd))                 //do handshaking of client with server
        Shminit();
    else
        printf("can't hand shake so can't Shm init, sorry :(\n");

    command_get_send(sockfd);               //command giver and sender
    
    return (EXIT_SUCCESS);
}

void sharememprinter()                      //print what all is in ut
{
    printf("\nServer said\n------------------------\n%s\n\nbaziar@client : ", shm);
    fflush(stdout);
}

void command_get_send(int sockfd)       //peigham girande va ...
{
    int lent;
    char buffer[201];
    char* command = (char*)malloc(sizeof(char) * MAX_LINE_LENGHT);

    while(1)
    {
        //bzero(command,MAX_LINE_LENGHT);
        //bzero(buffer,MAX_LINE_LENGHT);
        printf("baziar@client : ");
        if(getline(&command,&lent,stdin) == EOF)    //ctrl + D handling
			break;
        //printf("residam inja\n");
        if(strncmp(command,"exit",4) == 0)          //exit
        {
            write(sockfd, command, strlen(command));
            printf("client exited now...!\n");
            break;
        }
        else
        {
            //printf("alan mikham write konam\n");
            write(sockfd, command, strlen(command));
            //printf("alan mikham write konam kardam inam commandesh : %s\n",command);

            int n = 200;
            while(n == 200)
            {
                bzero(buffer,200);
                n = read(sockfd,buffer,200);
                printf("%s\n",buffer);
            }

//            printf("az while e sending biroon umadam");
//            printf("\n");
        }

    }

    free(command);
    close(sockfd);
    printf("**************server closed connection**************\n");
}

int handshaking(int sockfd)
{
    char* temp;
    char* buffer = (char*)malloc(256*sizeof(char));
    char* tt;
    int temp_pid = getpid();
    printf("and now handshaking...\n");
    temp = (char*) malloc(256*sizeof(char));

    sprintf(temp,"%d",temp_pid);            //say its pid to server
    printf("here is pid from socket : %s\n", temp);
    write(sockfd, temp, strlen(temp));

    bzero(temp,256);                        //send a unique message to server
    strcpy(temp,"I'm client of CA2 :D-(=:");
    write(sockfd, temp, strlen(temp));
    printf("man kiam.\n");
    bzero(buffer,256);

    read(sockfd,buffer,256);   //read shm key
    key = (key_t) atoi(buffer);
    printf("server shm key is : %d\n",key);
    //inja bayad client "man client e CA2 hastam" + Pid bede beserver
    //va key e share memory ro begire
    //if(any error) return0;
    printf("*******handshaking finished successfully*******\n");
    return 1;
}

int firstconnect()                  //connectiong steps
{
    int sockfd;
	char buffer[256];
	struct sockaddr_in dest_addr;
	if( (sockfd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
	{
		printf("systom fosh dad...\n");
		exit (0);
	}
	printf("ohaaaa...\n");
	bzero((char *) &dest_addr, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(PORT_NO);
	dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    while(1)
	{
		if( connect(sockfd, &dest_addr, sizeof(dest_addr)) < 0 )
		{
			printf("connection failed :((\n");
            return -1;
		}
		else
		{
			printf("HiH... we are connected :D\n");
			return sockfd;
		}
	}
}


void Shminit()          //initialize shared memory TV
{
    char c;
    int shmid;

    //create segment
    if ((shmid = shmget(key, SHMSZ, 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    //attach segment to client's data space
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    printf("*******shared memory attached successfully*******\n");
}
