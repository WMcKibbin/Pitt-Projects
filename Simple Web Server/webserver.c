#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#define MYPORT 50027
#define REQUESTSIZE 1024
//Length of response without content size integer length
#define RESPONSE_HEADER 115 
#define THREAD_LIMIT 50
int integerLength(int num);
void *htmlRequest(void* p);
int main(void)
{
	//create pthread/connfd arrays
	pthread_t pthreadArray[THREAD_LIMIT];
	int connfdArray[THREAD_LIMIT];
	int threadCount = 0;
	//initialize socket
        int sfd;
        struct sockaddr_in addr;
        sfd = socket(PF_INET, SOCK_STREAM, 0);
        if(sfd < 0)
        {
                printf("Socket could not intialize\n");
                return -1;
        }
	addr.sin_family = AF_INET;
        addr.sin_port = htons(MYPORT);
        addr.sin_addr.s_addr = INADDR_ANY; //auto find IP
//Bind and check for error
        int id = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
        if(id < 0)
	{
		printf("Socket could not bind.\n");
		return -1;
	}
//BEGIN INFINITE LOOP USE CTRL^C to shutdown

while(1)
{
	//Listen for a request
	if(listen(sfd, 10) < 0)
        {
                printf("Listen error.\n");
                return -1;
        }
//accept the connection
    if(threadCount < THREAD_LIMIT)
    {
		connfdArray[threadCount] = accept(sfd, NULL, NULL);
		if(connfdArray[threadCount] < 0)
		{
			printf("Could not accept connection.\n");

		}
		else
		{
			printf("Connection Established.\n");
			pthread_create(&(pthreadArray[threadCount]), NULL, htmlRequest, &(connfdArray[threadCount]));
			threadCount++;
		}
	}
	else
	{
		printf("Server at capacity. A request was ignored.\n");
	}
        
}//END INFINITE LOOP
    close(sfd);
}//END MAIN
///////////////////////////////////////////////
//START MULTI THREAD
///////////////////////////////////////////////
void *htmlRequest(void* p)
{
	char errorBuffer[1024];
	char requestBuffer[150];
	int requestSize = 0;
	//connection descripter
	int connfd = *((int*)p);

//recieve the request
	recv(connfd, requestBuffer, REQUESTSIZE, 0);
	printf("Request recieved.\n");
	printf("%s\n",requestBuffer);
	printf("Read this many bytes: %d\n", requestSize);
//Parse out the request
	//if the request matches our format
	char fileName[61] = {0};
	int fileNameSize = 0;
	if(!strncmp(requestBuffer, "GET /", 5))
	{
		int i = 0;
		char* current = requestBuffer + 5;
		while((*current != ' ') && (fileNameSize < 60))
		{
			fileName[i] = *current;
			current++;
			i++;
		}
		//Change the space to a null terminator
		fileName[i] = '\0';
		fileNameSize = i;
	}
	//if the request was the wrong format
	else
	{
		printf("Unknown Request Type.\n");
		close(connfd);
	}
//Attempt to open and read the file
	FILE* htmlFile = fopen(fileName, "r");
	int fileSize;
	char* fileBuffer = NULL;
	char* responseBuffer = NULL;
	if(htmlFile != NULL)
	{
		//get the size of the file
		fseek(htmlFile, 0, SEEK_END);
		fileSize = ftell(htmlFile);
		fseek(htmlFile, 0, SEEK_SET);
		//allocate a buffer of size fileSize
		fileBuffer = (char*)malloc(fileSize * sizeof(char));
		//fill that buffer with the file
		fread(fileBuffer, sizeof(char), fileSize, htmlFile);
		responseBuffer = (char*)malloc((fileSize + RESPONSE_HEADER + integerLength(fileSize)) * sizeof(char));
	}
	//File does not exist 404 not found error
	else
	{
		strcpy(errorBuffer,"HTTP/1.1 404 File Not Found\n");
		send(connfd, errorBuffer, strlen(errorBuffer), 0);
		close(connfd);
	}
//Construct the response
	//Date and Time vars
	char dateBuffer[40];
	time_t curTime;
	struct tm* gmtTime;
	//Send/packet vars
	int dataSent = 0;
	int responseBufferSize;
	//make sure we allocated memory
	if(responseBuffer && fileBuffer && connfd)
	{
	//Http header
		strcpy(responseBuffer,"HTTP/1.1 200 OK\n");
	//Date & Time
		strcat(responseBuffer,"Date: ");
		curTime = time(NULL);
		gmtTime = gmtime(&curTime);
		strftime(dateBuffer, 40, "%a, %d %b %Y %X %Z", gmtTime);
		strcat(responseBuffer, dateBuffer );
		strcat(responseBuffer, "\n");
	//Content-Length
		sprintf(responseBuffer,"%sContent-Length: %d\n", responseBuffer, fileSize);
	//Connection type and Content Type
		strcat(responseBuffer,"Connection: close\nContent-Type: text/html\n\n");
	//The html File
		strcat(responseBuffer, fileBuffer);
		strcat(responseBuffer, "\n\0");
//Respond to the request
	printf("Sending response:\n%s\n", responseBuffer);
	//need to accomadate multiple packets
	responseBufferSize = strlen(responseBuffer);
	int sendSize;
		while(dataSent < responseBufferSize)
		{
			if((sendSize = send(connfd, responseBuffer+dataSent, responseBufferSize - dataSent, 0)) < 0)
			{
				printf("Send Error\n");
			}
			else
			{
				dataSent += sendSize;
			}	
		}
	}
	else if((responseBuffer == NULL) || (fileBuffer == NULL))//Memory allocation error
	{
		printf("\nresponseBuffer: %d\nfileBuffer: %d\n", responseBuffer, fileBuffer);
		printf("\nMemory allocation error.\n");
		strcpy(errorBuffer,"Server Error. Try again later\n");
		send(connfd, errorBuffer, strlen(errorBuffer), 0);
		close(connfd);
	}
//the File needs to be mutex
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//Create the statistics buffer
	char* statsTxtBuffer = NULL;
	char* current;
	statsTxtBuffer = (char*)malloc(strlen(fileName)+ 64);
	current = requestBuffer;
	if(statsTxtBuffer)
	{	
		int i = 0;
		while(*current != '\n')
		{
			statsTxtBuffer[i] = *current;
			current++;
			i++;
		}
			statsTxtBuffer[i] = '\n';
		sprintf(statsTxtBuffer,"%sHost: localhost\nClient: 127.0.0.1:%d\n\0",statsTxtBuffer, MYPORT);
	}
//Begin appending the stats.txt file
	FILE* statsTxtFile = NULL;
	pthread_mutex_lock(&mutex);
	statsTxtFile = fopen("stats.txt","a");
	if(statsTxtFile && statsTxtBuffer)
	{
		fwrite(statsTxtBuffer, sizeof(char), strlen(statsTxtBuffer), statsTxtFile);
		fclose(statsTxtFile);
	}
	else
	{
		printf("stats.txt was not successfully opened\n\n");
	}
	pthread_mutex_unlock(&mutex);
//Clean up heap variables
	if(statsTxtBuffer)
		free(statsTxtBuffer);
	if(fileBuffer)
		free(fileBuffer);
	if(responseBuffer)
		free(responseBuffer);
//close the connection if it wasnt terminated by an error
	if(!connfd)
		close(connfd);
	return NULL;
}//END htmlRequest   
///////////////////////////////////////////////
//END MULTI THREAD
///////////////////////////////////////////////
////////////////////////////////////////////////////////////////
int integerLength(int num)
{
	return ((int)log10(num)) + 1;
}
