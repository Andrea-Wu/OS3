#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include "libnetfiles.h"

//this is to make sure if netserverinit() was called if it wasnt none of the other codes should work!
int netInitServerFlag=0;
struct addrinfo clientSideData;
struct addrinfo *serverSideData;
char ipAddressArray[INET_ADDRSTRLEN];

//connection method that is used for all the net file methods
int connectionForClientRequests(int socketDescript)
{
	//First create a socket
    	socketDescript=socket(serverSideData->ai_family, serverSideData->ai_socktype, serverSideData->ai_protocol);
	//Establish a connection for socket
    	int connectStatus=connect(socketDescript, serverSideData->ai_addr,serverSideData->ai_addrlen);
	//Check if socket was able to be connected
	if(connectStatus!=0)
	{
        	perror("ERROR: Unable to establish proper connection!\n");
        	return -1;
    	}
	inet_ntop(serverSideData->ai_family,(void *)((struct sockaddr *)serverSideData->ai_addr),ipAddressArray,sizeof(ipAddressArray));
	return socketDescript;
}

int handleNetOpenRequests(int socketDescriptor,const char* pathname,int flags)
{
        int openRequest=htonl(NETOPEN);
        if(send(socketDescriptor,&openRequest,sizeof(openRequest),0)==-1)
        {
                perror("ERROR: NetOpen request could not read the message!\n");
        }
        sleep(1);
	printf("NetOpen: Sending Path.\n");
        if(send(socketDescriptor,pathname,strlen(pathname),0)==-1)
        {
                perror("ERROR: NetOpen request could not send path!\n");
        }
        sleep(1);
	//sending flags to server side to handle client requests
        printf("NetOpen: Sending Flags\n");
        int flagsForRequest=htonl(flags);
        if(send(socketDescriptor,&flagsForRequest,sizeof(flagsForRequest),0)==-1)
        {
                perror("ERROR: NetOpen request could not send flags to server!");
        }
        //printf("NetOpen: waiting to receive result.\n");
        int resultFileDescriptor=0;
        int resultMessageFromServer=0;
	//Receiving data back from the server side, check whether or not server side returns an error
        if((resultMessageFromServer=recv(socketDescriptor,&resultFileDescriptor,sizeof(resultFileDescriptor),0))==-1)
        {
                perror("ERROR: NetOpen request could not receive result from server!\n");
                exit(-1);
        }
        printf("NetOpen: recieved result FD!\n");
        int resultFromServer=resultFileDescriptor;
        int errorTypeMessage=0;
	//Now that we received a file descriptr make sure it does not equal -1, which means there is an error!
	if(resultFromServer==-1)
        {
		//NetOpen fails to receive errno from server
                if((resultFromServer=recv(socketDescriptor,&errorTypeMessage,sizeof(errorTypeMessage),0))==-1)
                {
                         perror("ERROR: NetOpen request does not receive errno!\n");
                }
		//Checking errno and whether permission was denied!
                if(errorTypeMessage==DENIED_ACCESS)
                {
                        errno=13;
                        perror("ERROR: NetOpen request could not open file due to restrictions.");
                }
                else
                {
                        errno=errorTypeMessage;
                        perror("ERROR from server side!");
                }
        }
	//close the socket
        close(socketDescriptor);
	//return the result
        if(resultFromServer==-1)
        {
                 return resultFromServer;
        }
        return -1*resultFromServer;
	
}
//sends path name and read write flags to the server to be evaluated
int netopen(const char *pathname, int flags)
{
	//Make sure that netInitServer is called first before any operation is requested such as NetOpen,NetRead,NetWrite, and NetClose
    	if(netInitServerFlag==0)
	{
        	fprintf(stderr, "NetOpen request was called before netserverinit()!\n");
        	exit(-1);
    	}
    	int sockDescriptor=-1;
	//estbalish a connection for the following socket descriptor
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("NetOpen: Connected to %s\n",ipAddressArray);
   	sleep(1);
    	/***Make a seperate method for sending pathname and flags from the code below!***/
	printf("NetOpen: Sending File Mode.\n");
	int netOpenRequestResults=0;
	netOpenRequestResults=handleNetOpenRequests(sockDescriptor,pathname,flags);
	return netOpenRequestResults;
}

//in this method we have a buffer and the required bytes we want to read from the opened file
ssize_t netread(int filedes, void *buf, size_t nbyte)
{
    	if(netInitServerFlag==0)
	{
        	fprintf(stderr, "NetRead request was called before netserverinit()!\n");
        	exit(-1);
    	}
	int sockDescriptor=-1;
	//Establish a connection for the NetRead requested
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("Netread: Connected to %s\n", ipAddressArray);
	//For Thread synchronization
	sleep(1);
	printf("NetRead: Sending File Mode\n");
	
/*
	int messageOpenRequest=htonl(NETOPEN);
	//send data to server side with the message being NetOpen, since we must open the file before we read it
	if(send(sockDescriptor,&messageOpenRequest,sizeof(messageOpenRequest),0)==-1)
	{
        	perror("NetOpen request fails");
    	}
	sleep(1);
*/
    	int netreadMessage=htonl(NETREAD);
	//send data to server side with message being NetRead
    	if(send(sockDescriptor,&netreadMessage,sizeof(int),0)==-1)
	{
        	perror("NetRead request fails");
    	}

    	sleep(1);
	printf("NetRead: Sending File Descriptor.\n");
    	int filedescriptorNetReadRequest=htonl(filedes);
	//sending over the file descriptor to the server side in order to read the file
    	if(send(sockDescriptor,&filedescriptorNetReadRequest,sizeof(int),0)==-1)
	{
        	perror("NetRead request fails");
    	}
	sleep(1);
    	int sizeToRead=htonl(nbyte);
    	if(send(sockDescriptor,&sizeToRead,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetRead could not send the bytes over to be read to the server");
    	}
	//Here we set up to receive the result from the server
	printf("NetRead: waiting to receive result\n");
    	int resultSize=0;
    	int resultMessage=0;
    	if((resultMessage=recv(sockDescriptor,&resultSize,sizeof(resultSize),0))==-1)
	{
        	perror("ERROR: NetRead request could not read receive the results");
    	}
	int result=resultSize;
    	printf("NetRead: Received result size: %d\n", result);
	//Check for possible errors from NetRead requests
    	if(result==-1)
	{
        	int errorMessage=0;
		if((resultMessage=recv(sockDescriptor,&errorMessage,sizeof(errorMessage),0))==-1)
		{
            		perror("NetRead requests receives an error");
        	}
		errno=errorMessage;
		//set the value of errno
        	printf("Errno Is: %s\n",strerror(errno));
    	}
    	else
	{
        	//char resultString[MAXBUFFERSIZE];
        	if(recv(sockDescriptor,buf,nbyte,0)==-1)
		{
            		perror("NetRead requests receives error from server");
        	}
    	}
	
    	close(sockDescriptor);
    	return result;
}

//in this method we have a buffer and the required bytes we want to write from the opened file
ssize_t netwrite(int filedes,const void *buf,size_t nbyte)
{
	//check the whether or not netserverInit was called, this must be called befre a netopen,netread,netwrite,netclose is abl to perform
    	if(netInitServerFlag==0)
	{
        	fprintf(stderr, "NetWrite request called before netserverinit()\n");
        	exit(-1);
    	}
    	int socketDescriptor=-1;
	//establish a connection for the NetWrite Requst
    	socketDescriptor=connectionForClientRequests(socketDescriptor);
    	printf("netwrite: Connected to %s\n",ipAddressArray);
	sleep(1);
	//convert to a 32-bit integer host byte order and send to server
    	int netRequest=htonl(NETWRITE);
    	if(send(socketDescriptor,&netRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send message to server!\n");
    	}
	sleep(1);
	printf("NetWrite request sending over file descriptor to server\n");
    	int filedescriptorWriteRequest=htonl(filedes);
	//Sending over the data to the client
    	if(send(socketDescriptor,&filedescriptorWriteRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over the file descriptor to the server!\n");
    	}

    	sleep(1);
	//sending byte size to the client
    	//printf("netwrite: Sending nbyte to Server: %d\n", nbyte);
    	int sizeOfRequest=htonl(nbyte);
    	if(send(socketDescriptor,&sizeOfRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over the bytes to the server!\n");
    	}
	sleep(1);
	//Send String to the client 
    	//printf("netwrite: Sending String.\n");
    	if(send(socketDescriptor,buf,strlen(buf),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over string message to the server!\n");
    	}
    	int resultSize=0;
    	int resultMessage=0;
    	if((resultMessage=recv(socketDescriptor,&resultSize,sizeof(resultSize),0))==-1)
	{
        	perror("ERROR: NetWrite request could not receive results from the server!\n");
    	}
	int result=ntohl(resultSize);
    	printf("NetWrite: Received result size of: %d\n", result);
	//check result and see if any errors
    	//printf("netwrite: checking for errors.\n");
    	if(result==-1)
	{
        	int errorResult;
		if((resultMessage=recv(socketDescriptor,&errorResult,sizeof(errorResult),0))==-1)
		{
            		perror("netwrite received an error!");
        	}
		errno=ntohl(errorResult);
		perror("NetWrite request received errno!\n");
	}
	//close the scoket descriptor
    	close(socketDescriptor);
    	return result;
}

//in this method we want to close the file we opned earlier 
int netclose(int fd)
{
	
	if(netInitServerFlag==0)
	{
        	fprintf(stderr, "NetClose request called before netserverinit()\n");
        	exit(-1);
    	}
	int socketDescriptor=-1;
    	socketDescriptor=connectionForClientRequests(socketDescriptor);
    	printf("netclose: Connected to %s\n",ipAddressArray);
	sleep(1);
    	int messageCloseRequest=htonl(NETCLOSE);
	//convert into 32 bit integer host byte order and send to server
    	if(send(socketDescriptor,&messageCloseRequest,sizeof(int),0)==-1)
	{
        	perror("Error: NetClose cannot send message over");
    	}
	sleep(1);
    	printf("Netclose request is sending File Descriptor!\n");
    	int fileDescriptorRequest=htonl(fd);
    	if(send(socketDescriptor,&fileDescriptorRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetClose request was not able to send over the file descriptor");
    	}
	//receive results
    	int resultClose;
    	int resultMessage;
	//Get the results from the server side and check whether or not we were able to receive the results 
	if((resultMessage=recv(socketDescriptor,&resultClose,sizeof(resultClose),0))==-1)
	{
        	perror("ERROR: NetClose request could not receive results from server!\n");
    	}
    	int result=resultClose;
    	printf("Netclose request result: %d\n",result);
    	if(result==-1)
	{
         	int errorNumber;
		if((resultMessage=recv(socketDescriptor,&errorNumber,sizeof(errorNumber),0))==-1)
		{
            		perror("netread received an error");
        	}
		errno=ntohl(errorNumber);
       		perror("NetClose request has a problem!\n");
    	}
	//close the socketDescriptor
	close(socketDescriptor);
    	return result;

}
/*
 * This method is primarily used to one validate if a hostname is valid 
 * and check whether or not has either 
 * UNRESTRICED, EXCLSUIVE, OR TRANSACTION Mode for a file
*/ 
int netserverinit(char *hostname)
{
    	//printf("CALLING FUNCTIN OF THIS TYPE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    	int setSocketDescriptor=-1;
    	int status=0;
	memset(&clientSideData,0,sizeof(clientSideData));
    	clientSideData.ai_family=AF_INET;
    	clientSideData.ai_socktype=SOCK_STREAM; //TCP socket stream
    	clientSideData.ai_flags=AI_PASSIVE;
	//Checks that host-name exists
    	printf("Reviewing If Address Info Is Valid For: %s\n", hostname);
    	if((status=getaddrinfo(hostname,PORT,&clientSideData,&serverSideData))!=0)
	{
        	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		h_errno = HOST_NOT_FOUND;
        	return -1;
    	}
	printf("%s\n","Success! Hostname found!");
	netInitServerFlag=1;
    	return 0;
}

