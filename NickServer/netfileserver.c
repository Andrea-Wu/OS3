#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "libnetfiles.h"
#include <pthread.h>
#include <stdbool.h>

//Initialize an array of file descriptors of all to 0, our fdArray is 512
int fdArray[512]={0};
//hold the count of the number of file descriptor within fdArray
int countFileDescriptor=0;
//create the head of our FileDescriptorTable which is a linked list 
FileDescriptorTable* tableFD=NULL;
//Create a mutex for syncrhonization
pthread_mutex_t userListMutex=PTHREAD_MUTEX_INITIALIZER;

//This method is primarily used to insert each client thread into our file descriptor table whenever netOpen request is called from the client side
void insertLinkedList(FileDescriptorTable *packetInsertion)
{
	FileDescriptorTable* currentTable=tableFD;
	FileDescriptorTable* previousFDTable=NULL;
	//pthread_mutex_lock(&userListMutex);
	//Simle loop to get the correct positon to insert our new client when netOpen request is called
	while(currentTable!=NULL)
	{
		previousFDTable=currentTable;
		currentTable=currentTable->next;
	}
	//When currentTable is NULL, since we have the previous position of the file descriptor 
	//we simply set previousFDTable->next to the packet currently being inserted 
	
	if(previousFDTable == NULL){
		tableFD = packetInsertion;
	}
	else
	{
		previousFDTable->next=packetInsertion;
	}
	//pthread_mutex_unlock(&userListMutex);
}

//This method is primarily used to delete each client thread from our file descriptor table whenever netClose request is called from the client side
void removeNode(clientPacketData* target)
{
    	FileDescriptorTable *currentFDTable=tableFD;
    	FileDescriptorTable *previousFDTable=NULL;
	//for syncrhonization pruposes since the filde descriptor table is a shared data segment 
	pthread_mutex_lock(&userListMutex);
	while(currentFDTable!= NULL)
	{
		//when the node to be deleted is equal to the same as the current node we are at then delete
		if(target->serverFileDescriptor==currentFDTable->packetData->serverFileDescriptor)
		{
			//if only one node in the file descriptor possibility
	    		if(currentFDTable->next==NULL)
			{
				//check to see if the previousFDTable is null if so then we conclude that their is only one node in the File Descriptor Table, so we set tableFD to NULL
				if(previousFDTable==NULL)
				{
		    			tableFD=NULL;
	            			break;
				}
				//if the current file descriptor node is at the end of the linked list, then set the previousFDTable->next to be NULL
            			previousFDTable->next=NULL;
	        		break;
        		}
			//if more than one node and the node to be deleted is the head, then simply set tableFD to be equal to tableFD->next which becomes the new head of the File Descriptor Table
	    		if(previousFDTable==NULL)
			{
				tableFD=tableFD->next;
				break;
        		}
			//Set previousFDTable->next to the node to be deleted next value which is currentFDTable->next
        		previousFDTable->next=currentFDTable->next;
        		break;
      		}
		//increment the preiviousFDTable to currentFDTable 
		//and currentFDTable to currentFDTable->next
      		if(previousFDTable==NULL)
		{
          		previousFDTable=currentFDTable;
          		currentFDTable=currentFDTable->next;
          		continue;
      		}
		
      		previousFDTable=previousFDTable->next;
      		currentFDTable=currentFDTable->next;
    	}
	//unlock for synchronization purposes so we don't encounter a deadlock!
	pthread_mutex_unlock(&userListMutex);
}

//opens file on the remote machine and returns the FD and any errno. It also checks if the files has been used as well.
clientPacketData* handleOpenRequest(clientPacketData* packet,char buffer[MAXBUFFERSIZE],int errorNumber)
{
	int validPath=0;
	//First check whether the file name given from the client request NETOPEN is valid 
	//recv from essentially receives the data from the client side
	//If -1 then we know that the path could not be received and is invalid so we simply exit with exit(1)
      	if((validPath=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1)
	{ 
		//getting file name to open
        	perror("NetOpen: Could not receive path");
        	exit(1);
      	}
	buffer[validPath]='\0';
	//If valid path them simply print the path to the file
      	printf("NetOpen: Received path: %s\n", buffer);
	//Initialize the metadata from the client(packet)'s filename (filed within the struct) to validPath
      	packet->fileName=malloc(validPath);
	//copy the name of the filename into the buffer
     	strcpy(packet->fileName, buffer);
	//check the flags received this means flags such as O_RDONLY,O_WRONLY,O_RDWR...etc
      	int flagsReceived=0;
      	int checkFlag=0;
	//if the falgs received from the client side is equal to -1
	//then print error message saying we have an issue recing flags from client 
      	if((checkFlag=recv(packet->clientFileDescriptor,&flagsReceived,sizeof(int),0))==-1)
	{
        	perror("ERROR: Netopen request has an issue in receiving flags!\n");
      	}
	//Convert the flagsReceived into a 32-bit integer in host byte order this is used for data exchange with the method ntohl
      	int flags=ntohl(flagsReceived);
      	printf("NetOpen: Received flags: %i\n",flags);
	//initialize the packet field modeFlags with flags
      	packet->modeFlags=flags;
	// try actually opening the file and then sending the result FD back
      	printf("NetOpen: Trying to open the file\n");
      	int result=0;
      	result=open(buffer,flags);
	//Check the result of open 
	//If not -1, then we know that the file was able to open successfully 
	//and we return the negative version of the server file descriptor back to the client side
      	if(result!=-1)
      	{
          packet->serverFileDescriptor=-1*result;
      	}
	//send over the data the server processed back to the server side!
	//first check to see if send does not return -1
	int currentResult=0;
      	currentResult=result;
	//if so then we have a bad file descriptor
      	if (send(packet->clientFileDescriptor,&currentResult,sizeof(int),0)==-1)
      	{
        	perror("ERROR: NetOpen request has received a bad file descriptor!\n");
      	}
      	//if there was an error getting the resulting FD
      	if(result==-1)
      	{
		//Could not send data back to the client properly so we send errno
        	if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
		{
           		perror("NetOpen: issue with sending errno");
        	}
	}
       //check the count of the file descriptor and make sure its less than the length of the fdArray and set the countFileDescriptor to be equal to the current file descriptor
       
      	pthread_mutex_lock(&userListMutex);
	FileDescriptorTable* newPacket = (FileDescriptorTable*)malloc(sizeof(FileDescriptorTable));
	newPacket->packetData = packet;
	newPacket->next = NULL;
	insertLinkedList(newPacket);
	pthread_mutex_unlock(&userListMutex);
       
	if(countFileDescriptor<512)
	{
		fdArray[countFileDescriptor]=currentResult;
	}
	else
	{
		printf("ERROR: NetOpen request has received too many files too open.\n");
		countFileDescriptor++;
	}
	return packet;
}

//From using the file descriptor above in the open method above we read how many bytes the client want into their buffer.
clientPacketData* handleReadRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber)
{

      	int fdReceiver=0;
      	int fdMessage=0;
	//Check the data received from the NetRead request and see whether or not is wable to receive the message by checking to see if recv returns -1
	if((fdMessage=recv(packet->clientFileDescriptor, &fdReceiver,sizeof(fdReceiver),0))==-1)
	{
        	perror("ERROR: Netread request could not receive the message");
      	}
	int readfd=0;
	//convert into 32-bit integer host byte order
      	readfd=ntohl(fdReceiver);
      	printf("NetRead: Received File Descriptor: %d\n",readfd);
	//check if the read file descriptor is equal to -1, if so then a error occured!
      	if(readfd==-1)
	{
		readfd=-1;
      	}
	//otherwise valid file descriptor 
	//send client back negative version of the server side file descriptor
      	else if(readfd<0)
	{
		printf("The read file descriptor is: %d\n", readfd);
        	readfd=readfd*-1;
      	}
      	size_t currentBytesReceived;
      	int messageBit;
	//check the data received from the NetRead request from client and see whether or not it returns -1, if so print error message
	if((messageBit=recv(packet->clientFileDescriptor,&currentBytesReceived,sizeof(currentBytesReceived),0))==-1)
	{
       		perror("ERROR: Netread request could not receive any number of bytes from client");
      	}
	//convert into 32-bit integer host byte order
	size_t numBytesToBeRead=ntohl(currentBytesReceived);
      	printf("NetRead: Received nbytes that are to be read: %d\n", numBytesToBeRead);
      	
	off_t offsetBytesReceived;
	//check the data received from the NetRead request from client and see whether or not it returns -1, if so print error message
	if((messageBit=recv(packet->clientFileDescriptor,&offsetBytesReceived,sizeof(offsetBytesReceived),0))==-1)
	{
       		perror("ERROR: Netread request could not receive any number of bytes from client");
      	}
	//convert into 32-bit integer host byte order
	off_t numBytesToBeOffset =ntohl(offsetBytesReceived);
      	printf("NetRead: Received offset: %d\n", numBytesToBeOffset);
	sleep(1);
	//if the file descriptor is -1 
	//then bad file descriptor
      	if(readfd==-1)
	{
		//set bytes to be read to -1
		//send data packet to the client side and print error message if failed to send back to client
		int bytesReadResult=-1;
        	if(send(packet->clientFileDescriptor,&bytesReadResult,sizeof(bytesReadResult),0)==-1)
		{
			perror("ERROR: Netread request has an issue with sending result!");
        	}
        	//send errno, with 9 indicating this is a bad file number
		errno=9;
        	printf("NetRead: Sending errno :%d\n", errno);
		//set the errorNumber to be equal to errno
        	errorNumber=errno;
		//send data back to the client side
        	if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
		{
          		perror("NetRead: was not able to send errno");
        	}
        	return packet;
	}
	//now send over the buffer that was currently read in back to the client side
	char outputBuffer[MAXBUFFERSIZE];
	//first bzero out the buffer
      	bzero(buffer, MAXBUFFERSIZE);
	
      	int readResult=0;
      	int currentBitsReadInFlag=0;
      	int currentByesResult=0;
      	pthread_mutex_lock(&userListMutex);
////////New for offset
	lseek(readfd, numBytesToBeOffset, SEEK_SET);
	
	//shared data segment so lock the critical section
	//check if the numBytesToBeRead is greater than the MAXBUFFERSIZE which is 512
      	if(numBytesToBeRead>MAXBUFFERSIZE)
      	{
		//pthread_mutex_lock(&userListMutex);
      		while(numBytesToBeRead!=0)
	  	{
			if(numBytesToBeRead<MAXBUFFERSIZE)
			{
				//update the value of currentBitsReadFlag from the read function 
            			currentBitsReadInFlag+=read(readfd,outputBuffer,numBytesToBeRead-1);
				//append the end of the outputerBuffer to a null termianting character
            			strcat(outputBuffer,"\0");
				//updated the value of readResult
            			readResult=readResult+currentBitsReadInFlag;
		    		readResult++;
				//updated the value of numBytesToBeRead so we don't overwrite
           			numBytesToBeRead=numBytesToBeRead-currentBitsReadInFlag;
				//validate currentBytesResult flag
            			currentByesResult=1;
            			break;
          		}
          		if(numBytesToBeRead>MAXBUFFERSIZE)
			{
				readResult=readResult+read(readfd,buffer,MAXBUFFERSIZE);
            			numBytesToBeRead=numBytesToBeRead-MAXBUFFERSIZE;
			}
        	}
      }
      else
      {
		//pthread_mutex_lock(&userListMutex);
		//If numBytesToBeRead is NOT greater than MAXBUFFERSIZE than simple check read the values from the buffer
        	readResult=read(readfd,buffer,numBytesToBeRead);
      }
      //unlock the critical section
      pthread_mutex_unlock(&userListMutex);
      printf("NetRead: Buffer Result: %s\n", buffer);
      int bytesReadResult=readResult;
      //send back the results back to the client
      if(send(packet->clientFileDescriptor, &bytesReadResult,sizeof(bytesReadResult),0)==-1)
      {
       	 perror("ERROR: NetRead request has an issue in sending result!");
      }
      //if there was an error getting the resulting size
      if(readResult==-1)
      {
	//send errno back to the client 
        printf("NetRead: Sending errno :%d\n", errno);
        errorNumber=errno;
	//Check whether or not the data to be sent back to the client is equal to -1
        if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
	{
		perror("ERROR: NetRead request could not send errno to client!");
        }
        return packet;
     }
     //if the currentBytesResult flag is validated meaning we actually read from the buffer from the NetRead request
     if(currentByesResult==1)
     {
	//check whether or not the output buffer is equal to the null terminating character
	if(strcmp(outputBuffer,"\0")==0)
	{
		printf("NetRead: Sending buffer: %s\n", buffer);
	    	if(send(packet->clientFileDescriptor,buffer,readResult,0)==-1)
		{
			perror("ERROR: NetRead request has an issue with sending the buffer back to the client");
          	}
        }
	else
	{
		printf("NetRead request sending buffer: %s\n", outputBuffer);
	    	if(send(packet->clientFileDescriptor,outputBuffer,currentBitsReadInFlag,0)==-1)
		{
			perror("ERROR: NetRead request has an issue with sending the buffer back to the client");
          	}
        }
     }
     else
     {
	printf("NetRead: Sending buffer: %s\n", buffer);
        if(send(packet->clientFileDescriptor,buffer,readResult,0)==-1)
	{
		perror("ERROR: NetRead request has an issue with sending the buffer back to the client");
        }
    }
    return packet;
}

//The write method takes in the amount of bytes the client wants to write and the buffer they passed in
clientPacketData* handleWriteRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber)
{
      	int fdReceived=0;
      	int fdMessage=0;
	//Check the data that comes from the client side and see whether or not an error occured
      	if((fdMessage=recv(packet->clientFileDescriptor,&fdReceived,sizeof(fdReceived),0))==-1)
      	{
		//return error message
        	perror("ERROR: NetWrite request was Unable to receive message with file descriptor\n");
      	}
	//convert into 32-bit integer host byte order
      	int writefd=ntohl(fdReceived);
      	printf("NetWrite: Received File Descriptor: %d\n", writefd);
	//if the result from nthol returns -1 then we have a bad file descriptor
      	if(writefd==-1)
      	{
		writefd=-1;
      	}
	//valid file descriptor! return the negative file descriptor back to the client 
      	else if(writefd<0)
	{
        	printf("%d\n", writefd);
        	writefd = writefd * -1;
      	}
      	size_t writenbytesReceived;
      	int writenbyteMessage;
	//check the number of bytes received from the client side to write into our buffer and see whether or not it returns -1, if so error
      	if((writenbyteMessage=recv(packet->clientFileDescriptor,&writenbytesReceived,sizeof(writenbytesReceived),0))==-1)
	{
       		perror("ERROR: NetWrite request was not able to receive the bytes from client side");
      	}
      	size_t writenbyte=ntohl(writenbytesReceived);
      	printf("NetWrite: Waiting for string to write in file...\n");
      	int stringMessage=0;
	//get the message to write into our buffer from the client and check whether or not the server received the message properly by whether or not it returns -1
      	if((stringMessage=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1)
	{
        	perror("ERROR: NetWrite did not receive a message to write from the client!\n");
        	exit(1);
      	}
	//set the last character to a null terminating character
      	buffer[stringMessage]='\0';
      	printf("NetWrite: Received string: %s\n", buffer);
      	sleep(1);
	
	off_t offsetBytesReceived;
	int messageBit;
	//check the data received from the NetRead request from client and see whether or not it returns -1, if so print error message
	if((messageBit=recv(packet->clientFileDescriptor,&offsetBytesReceived,sizeof(offsetBytesReceived),0))==-1)
	{
       		perror("ERROR: Netwrite request could not receive the offset from client");
      	}
	//convert into 32-bit integer host byte order
	off_t numBytesToBeOffset =ntohl(offsetBytesReceived);
      	printf("NetWrite: Received offset: %d\n", numBytesToBeOffset);
	sleep(1);


pthread_mutex_lock(&userListMutex);
      	//printf("NetWrite: Trying to write to the file\n");
      	lseek(writefd, numBytesToBeOffset, SEEK_SET);
	int writeresult=write(writefd, buffer, writenbyte);
pthread_mutex_unlock(&userListMutex);
      	int writeresultCurrent=htonl(writeresult);
      	if(send(packet->clientFileDescriptor,&writeresultCurrent,sizeof(writeresultCurrent),0)==-1)
	{
        	perror("ERROR: NetWrite request was Unable to send result back to the client!\n");
      	}
      	//if there was an error getting the resulting size
      	if(writeresult==-1)
	{
        	printf("NetWrite: Sending errno: %d\n", errno);
        	errorNumber=htonl(errno);
        	if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
		{
          		perror("ERROR: NetWrite: Issue with sending errno");
        	}
    	}
    	return packet;
}

//this method closes the FD in which was recieved from the open method and returns 0 if successfull
clientPacketData* handleCloseRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber)
{
	int receiveClientData=0;
    	int closeClientData=0;
      	if((closeClientData=recv(packet->clientFileDescriptor,&receiveClientData,sizeof(receiveClientData),0))==-1)
	{
		perror("ERROR: Netclose request was not able to receive file descriptor to close it!\n");
	}
	int fdClose=ntohl(receiveClientData);
	packet->serverFileDescriptor=fdClose;
	printf("Netclose request as able to receive file descriptor: %d\n",fdClose);
	if(fdClose>=-1) //definetly not a valid file descriptor 
	{
		int n=htonl(-1);
		if(send(packet->clientFileDescriptor,&n,sizeof(n),0)==-1) //let client know something happened 
		{
			perror("ERROR: Netclose request has issue with sending data/fd back to client!\n");
		}
		errno=EBADF;
		printf("Netclose request is sending errno: %d\n",errno);
		errorNumber=htonl(errno);
		//send the errno 
		if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
		{
			perror("ERROR: Netclose request has issue with sending errno!\n");
		}
	}
	else //potential fd to close
	{
		fdClose=fdClose*-1;
		sleep(1);
		int closed=close(fdClose);
		int closedFD=htonl(closed);
		pthread_mutex_lock(&userListMutex);
		int exists=0;
		FileDescriptorTable* currentTable=tableFD;
		//check if the fd exists in our record
		
		while(currentTable!=NULL)
		{
			if(currentTable->packetData->serverFileDescriptor==(fdClose*-1))
			{
				exists=1;
			}
			currentTable=currentTable->next;
		}
		pthread_mutex_unlock(&userListMutex);
		if(exists==0)
		{
			//let client know something happened, send errno 
			int n=htonl(-1);
			if(send(packet->clientFileDescriptor,&n,sizeof(n),0)==-1)
			{
				perror("Error: Netclose reuqest has issue with sending data/fd back to client!\n");
			}
			errno=EBADF;
			printf("Netclose is sending errno: %d\n",errno);
			errorNumber=htonl(errno);
			if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
			{
				perror("ERROR: Netclose request has issue with sending errno!");
			}
		}
		else //fd exists, let client know success
		{
			removeNode(packet);
			if(send(packet->clientFileDescriptor,&closedFD,sizeof(closedFD),0)==-1)
			{
				perror("Error: Netclose request has issue with sending data/fd back!");
			}
		}
	}
      	return packet;
}

clientPacketData* handleMkdirRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){
	

	int validPath=0;
      	if((validPath=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1)
	{ 
		//getting file name to open
        	perror("NetMkdir: Could not receive path");
        	exit(1);
      	}
	buffer[validPath]='\0';
      	printf("NetMkdir: Received path: %s\n", buffer);
      	packet->fileName=malloc(validPath);
	//copy the name of the filename into the buffer
     	strcpy(packet->fileName, buffer);
	//check the flags received this means flags such as O_RDONLY,O_WRONLY,O_RDWR...etc
      	int flagsReceived=0;
      	int checkFlag=0;
	//if the flags received from the client side is equal to -1
	//then print error message saying we have an issue recing flags from client 
      	if((checkFlag=recv(packet->clientFileDescriptor,&flagsReceived,sizeof(int),0))==-1)
	{
        	perror("ERROR: Netopen request has an issue in receiving flags!\n");
      	}
	//Convert the flagsReceived into a 32-bit integer in host byte order this is used for data exchange with the method ntohl
      	int flags=ntohl(flagsReceived);
      	printf("NetOpen: Received flags: %i\n",flags);
	//initialize the packet field modeFlags with flags
      	packet->modeFlags=flags;
	// try actually opening the dir and then sending the result FD back
      	printf("NetMkdir: Trying to open the file\n");
      	int result=0;
      	result=mkdir(buffer,flags);
	//Check the result of open 
	//If not -1, then we know that the file was able to open successfully 
	//and we return the negative version of the server file descriptor back to the client side
      	if(result!=-1)
      	{
          packet->serverFileDescriptor=-1*result;
      	}
	//send over the data the server processed back to the server side!
	//first check to see if send does not return -1
	int currentResult=0;
      	currentResult=result;
	//if so then we have a bad file descriptor
      	if (send(packet->clientFileDescriptor,&currentResult,sizeof(int),0)==-1)
      	{
        	perror("ERROR: NetMkdir request has received a bad file descriptor!\n");
      	}
      	//if there was an error getting the resulting FD
      	if(result==-1)
      	{
		//Could not send data back to the client properly so we send errno
        	if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
		{
           		perror("NetMkdir: issue with sending errno");
        	}
	}
       //check the count of the file descriptor and make sure its less than the length of the fdArray and set the countFileDescriptor to be equal to the current file descriptor
       
      	pthread_mutex_lock(&userListMutex);
	FileDescriptorTable* newPacket = (FileDescriptorTable*)malloc(sizeof(FileDescriptorTable));
	newPacket->packetData = packet;
	newPacket->next = NULL;
	insertLinkedList(newPacket);
	pthread_mutex_unlock(&userListMutex);
       
	if(countFileDescriptor<512)
	{
		fdArray[countFileDescriptor]=currentResult;
	}
	else
	{
		printf("ERROR: NetMkdir request has received too many files too open.\n");
		countFileDescriptor++;
	}

	return packet;
}

//function pointer for thread handler 
//this is used to get the message from the client side on whether they called 
//NETOPEN = 0
//NETREAD = 1
//NETWRITE = 2
//NETCLOSE = 3
void *clientRequestCalls(void *clientInfoRequest)
{
	clientPacketData *packet = (clientPacketData*)clientInfoRequest;
  	char buffer[MAXBUFFERSIZE];
	int errorNumber=0;
  	switch(packet->messageMode)
	{
		case NETOPEN: // every netopen call, we malloc one table position to include a client
			printf("NetOpen Request: IP Address %s\n", packet->ipAddress);
        		packet=handleOpenRequest(packet, buffer, errorNumber);
        		printf("NetOpen: Finished Operation.\n");
        		close(packet->clientFileDescriptor);
        		break;
		case NETREAD:
			printf("NetRead Request: IP Address %s\n",packet->ipAddress);
      			packet=handleReadRequest(packet, buffer, errorNumber);
      			printf("NetRead: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETWRITE:
			printf("NetWrite Request: IP Address %s\n",packet->ipAddress);
      			packet = handleWriteRequest(packet, buffer, errorNumber);
      			printf("NetWrite: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETCLOSE:
			printf("NetClose Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			printf("NetClose: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETCREATE:
			printf("NetCreate Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetCreate: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETFLUSH:
			printf("NetFlush Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetFlush: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETRELEASE:
			printf("NetRelease Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetRelease: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETTRUNCATE:
			printf("NetTruncate Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetTruncate: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETGETATTR:
			printf("NetGetattr Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetGetattr: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETOPENDIR:
			printf("NetOpendir Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetOpendir: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETREADDIR:
			printf("NetReaddir Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetReaddir: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETRELEASEDIR:
			printf("NetReleasedir Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCloseRequest(packet, buffer, errorNumber);
      			//printf("NetReleasedir: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETMKDIR:
			printf("NetMkdir Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleMkdirRequest(packet, buffer, errorNumber);
      			printf("NetMkdir: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
  	}
}

//the main method starts the server and sets all required parameters and then creates a packet and sends it through a pthread
int main(int argc, char * argv[])
{
	int socketFileDescriptor;
  	int serverFileDescriptor;
	// the struct addrinfo allows us to map the host name and service name to an address
  	struct addrinfo hints;
  	struct addrinfo *result;
	printf("Starting up netfileserver!\n");
	memset(&hints,0,sizeof(hints));
  	hints.ai_family=AF_INET; 
  	hints.ai_socktype=SOCK_STREAM; //FOR TCP CONNECTION
  	hints.ai_flags=AI_PASSIVE; //TCP CONNECTION
	getaddrinfo(NULL,PORT,&hints,&result);
  	socketFileDescriptor=socket(result->ai_family,result->ai_socktype,0);
  	printf("Binding socket descriptor to IP Address Info...\n");
  	bind(socketFileDescriptor, result->ai_addr, result->ai_addrlen);
	listen(socketFileDescriptor, 10);
	char currentBuffer[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN reperets sents the 16 integer length IP ADDRESS
  	struct sockaddr_storage clientAddress;
  	socklen_t addressSize;
  	while(1)
	{
    		//accept()waits for connections to come in to connect to
    		addressSize=sizeof(clientAddress);
		//accept new connections from the client
    		serverFileDescriptor=accept(socketFileDescriptor,(struct sockaddr *)&clientAddress,&addressSize);
		//if the serverFileDescriptor returns -1 than we have an error in accepting the connection from the client
    		if(serverFileDescriptor==-1)
		{
      			perror("Accept failed!\n");
    		}
		//inet_ntop puts the client's ip into a string to work with
    		inet_ntop(clientAddress.ss_family, (void *)((struct sockaddr *)result->ai_addr),currentBuffer,sizeof(currentBuffer));
    		printf("Client connected: %s\n", currentBuffer);
		//Initialize packets as well as fields for packet
    		clientPacketData* packet=malloc(sizeof(clientPacketData));
    		strcpy(packet->ipAddress,currentBuffer);
    		packet->clientFileDescriptor=serverFileDescriptor;
		int filemode=0;
    		int modemsg=0;
		//get the metadata from the client side
		//Receiving the first message from the client
    		int msg_type=0;
    		int msg=0;
    		if((msg=recv(serverFileDescriptor,&msg_type,sizeof(int),0))==-1)
		{
      			perror("ERROR: Issue with first receiving open/close/read/write code");
      			exit(1);
    		}
    		packet->messageMode=ntohl(msg_type);
    		printf("Received: %x from %s\n", packet->messageMode, currentBuffer);
		//This is used to create a thread ID
		pthread_t child;
		//spawn a new thread for each client request
    		pthread_create(&child, NULL, clientRequestCalls, (void *)packet);
  	}
  	return 0;
}
