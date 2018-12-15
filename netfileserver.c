#include "libnetfiles.h"

//Initialize an array of file descriptors of all to 0, our fdArray is 512
int fdArray[512]={0};
//hold the count of the number of file descriptor within fdArray
int countFileDescriptor=0;
//create the head of our FileDescriptorTable which is a linked list 
FileDescriptorTable* tableFD=NULL;
//Create a mutex for syncrhonization
pthread_mutex_t userListMutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t userDirMutex=PTHREAD_MUTEX_INITIALIZER;

//command line parameters
char* PORT;
char* MOUNTPATH;

//the directory linked list
dirNode* head;

char* getFilename(char* path){
    char* newPath = (char*)malloc(sizeof(char) * 100);
    strcpy(newPath, MOUNTPATH);
    strcat(newPath, path);
    return newPath;
}

//This method is primarily used to insert each client thread into our file descriptor table whenever netOpen request is called from the client side
void insertLinkedList(FileDescriptorTable *packetInsertion){
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

//opens file on the remote machine and returns the FD and any errno. It also checks if the files has been used as well.
clientPacketData* handleOpenRequest(clientPacketData* packet,char buffer[MAXBUFFERSIZE],int errorNumber){
	int validPath=0;
	//First check whether the file name given from the client request NETOPEN is valid 
	//recv from essentially receives the data from the client side
	//If -1 then we know that the path could not be received and is invalid so we simply exit with exit(1)
      	if((validPath=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){ 
		//getting file name to open
        	perror("NetOpen: Could not receive path");
        	exit(1);
      	}
	buffer[validPath]='\0';
	//If valid path them simply print the path to the file
	//Initialize the metadata from the client(packet)'s filename (filed within the struct) to validPath
      	packet->fileName=malloc(validPath);
	//copy the name of the filename into the buffer
     	strcpy(packet->fileName, buffer);
	//check the flags received this means flags such as O_RDONLY,O_WRONLY,O_RDWR...etc
      	int flagsReceived=0;
      	int checkFlag=0;
	//if the falgs received from the client side is equal to -1
	//then print error message saying we have an issue recing flags from client 
      	if((checkFlag=recv(packet->clientFileDescriptor,&flagsReceived,sizeof(int),0))==-1){
        	perror("ERROR: Netopen request has an issue in receiving flags!\n");
      	}
	//Convert the flagsReceived into a 32-bit integer in host byte order this is used for data exchange with the method ntohl
      	int flags=ntohl(flagsReceived);
	//initialize the packet field modeFlags with flags
      	packet->modeFlags=flags;
	// try actually opening the file and then sending the result FD back
      	int result=0;
      	buffer = getFilename(buffer);
	result=open(buffer,flags);
	//Check the result of open 
	//If not -1, then we know that the file was able to open successfully 
	//and we return the negative version of the server file descriptor back to the client side
      	if(result!=-1){
          packet->serverFileDescriptor=result;
      	}
	//send over the data the server processed back to the server side!
	//first check to see if send does not return -1
	int currentResult=0;
      	currentResult=result;
	//if so then we have a bad file descriptor
      	if (send(packet->clientFileDescriptor,&currentResult,sizeof(int),0)==-1){
        	perror("ERROR: NetOpen request has received a bad file descriptor!\n");
      	}
      	//if there was an error getting the resulting FD
      	if(result==-1){
		//Could not send data back to the client properly so we send errno
        	if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1){
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

clientPacketData* handleCreateRequest(clientPacketData* packet,char buffer[MAXBUFFERSIZE],int errorNumber){
	int validPath=0;
    if((validPath=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){ 
        //getting file name to open
        perror("NetCreate: Could not receive path");
        exit(1);
    }
	buffer[validPath]='\0';
	//If valid path them simply print the path to the file
	//Initialize the metadata from the client(packet)'s filename (filed within the struct)
    //to validPath

    packet->fileName=malloc(validPath);
	//copy the name of the filename into the buffer
    strcpy(packet->fileName, buffer);

	//check the flags received this means flags such as O_RDONLY,O_WRONLY,O_RDWR...etc
    int flagsReceived=0;
    int checkFlag=0;
	//if the falgs received from the client side is equal to -1
	//then print error message saying we have an issue recing flags from client 
    if((checkFlag=recv(packet->clientFileDescriptor,&flagsReceived,sizeof(int),0))==-1){
        perror("ERROR: Netopen request has an issue in receiving flags!\n");
    }
	//Convert the flagsReceived into a 32-bit integer in host byte order this is used for data exchange with the method ntohl
    int flags=ntohl(flagsReceived);
	//initialize the packet field modeFlags with flags
    packet->modeFlags=flags;
	// try actually opening the file and then sending the result FD back

    char* newPath = getFilename(buffer);
	//Check the result of open 
	//If not -1, then we know that the file was able to open successfully 
	//and we return the negative version of the server file descriptor back to the client side
    int result = 0;
    if((result = creat(newPath,flags))){
      packet->serverFileDescriptor=result;
    }else{
      perror("netCreate: create file failed => ");
    }
	//send over the data the server processed back to the server side!
	//first check to see if send does not return -1
	int currentResult=0;
  	currentResult=result;
	//if so then we have a bad file descriptor
    if (send(packet->clientFileDescriptor,&currentResult,sizeof(int),0)==-1){
        perror("ERROR: NetCreate request has received a bad file descriptor!\n");
    }
      	//if there was an error getting the resulting FD
    if(result==-1){
    //Could not send data back to the client properly so we send errno
        if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1){
            perror("NetCreate: issue with sending errno");
        }
	}
   //check the count of the file descriptor and make sure its less than the length of
   //the fdArray and set the countFileDescriptor to be equal to the current file descriptor
   
      	pthread_mutex_lock(&userListMutex);
	FileDescriptorTable* newPacket = (FileDescriptorTable*)malloc(sizeof(FileDescriptorTable));
	newPacket->packetData = packet;
	newPacket->next = NULL;
	insertLinkedList(newPacket);
	pthread_mutex_unlock(&userListMutex);
       
	if(countFileDescriptor<512){
		fdArray[countFileDescriptor]=currentResult;
	}else{
		printf("ERROR: NetCreate request has received too many files too open.\n");
		countFileDescriptor++;
	}
	return packet;
}


//From using the file descriptor above in the open method above we read how many bytes the client want into their buffer.
clientPacketData* handleReadRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){
    int msgReciever = 0;
    int offsetReciever = 0;
    if((msgReciever =recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){
        	perror("ERROR: read request could not receive the directory name");
    }else{
    }
    packet->fileName = malloc(msgReciever);
	//copy the name of the filename into the buffer
	strcpy(packet->fileName, buffer);

    if((offsetReciever=recv(packet->clientFileDescriptor, &offsetReciever,sizeof(offsetReciever),0))==-1){
        perror("ERROR: read request could not receive the offset");
    }else{
    }

    //get the proper directory name 
    char* newPath = getFilename(buffer);

    //setup to read into buffer
    memset(buffer, '\0', MAXBUFFERSIZE);

    //search up 

    int fd = open(newPath, O_RDONLY);
    if(read(fd, buffer, MAXBUFFERSIZE)){
    }else{
        perror("read: reading from %s file fails =>");
    }
    /*

typedef struct packet{  
  char ipAddress[INET_ADDRSTRLEN]; //character of the ipAddress for the network with INET_ADDRSTRLEN referring too the length of the string form for the IP Address
  int clientFileDescriptor; //file descriptor on the client side
  int messageMode; // associated with netread netwrite netopen netclose etc
  int serverFileDescriptor; //file descriptor used on the server side
  int modeFlags; //this is associated with O_RD, O_WR,O_RDONLY, etc
  char* fileName;
} clientPacketData;
    */

	packet->serverFileDescriptor = fd;
	pthread_mutex_lock(&userListMutex);
	FileDescriptorTable* newPacket = (FileDescriptorTable*)malloc(sizeof(FileDescriptorTable));
	newPacket->packetData = packet;
	newPacket->next = NULL;
	insertLinkedList(newPacket);
	pthread_mutex_unlock(&userListMutex);

    //send size of buffer back to the client
      //if(send(packet->clientFileDescriptor, buffer,sizeof(buffer),0)==-1)
    int bufferLen = strlen(buffer);
    if(send(packet->clientFileDescriptor, &bufferLen,sizeof(int),0)==-1){
       	 perror("ERROR: NetRead request has an issue in sending size result!");
    }
      
	//check whether or not the output buffer is equal to the null terminating character
	if(strcmp(buffer,"\0")==0){
	    	if(send(packet->clientFileDescriptor,buffer,bufferLen,0)==-1){
			    perror("ERROR: NetRead request broke while sending the buffer back to the client");
          	} 
    }else{
        if(send(packet->clientFileDescriptor,buffer,bufferLen,0)==-1){
		    perror("ERROR: NetRead request broke while sending the buffer back to the client");
        }
    }
    return packet;

}

//The write method takes in the amount of bytes the client wants to write and the buffer they passed in
clientPacketData* handleWriteRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){

    //receive the file name 
    int msgReciever = 0;
    if((msgReciever =recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){
        perror("write: server could not receive the file name => ");
    }else{
    }
    buffer[msgReciever] = '\0'; //not sure
    packet->fileName = malloc(msgReciever);
	//copy the name of the filename into the buffer
	strcpy(packet->fileName, buffer);

    //get the right file name, call it "newPath"
    char* newPath = getFilename(buffer);
    
    //receive byte size
    size_t writenbytesReceived;
    int writenbyteMessage;
    //check the number of bytes received from the client side to write into our buffer 
    //and see whether or not it returns -1, if so error
    if((writenbyteMessage=recv(packet->clientFileDescriptor,&writenbytesReceived,sizeof(writenbytesReceived),0))==-1){
        perror("ERROR: NetWrite request was not able to receive the bytes from client side");
    }
    size_t writenbyte=ntohl(writenbytesReceived);

    //receive string
    int stringMessage=0;
    //get the message to write into our buffer from the client 
    //and check whether or not the server received the message properly
    //by whether or not it returns -1
    if((stringMessage=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){
        perror("ERROR: NetWrite did not receive a message to write from the client!\n");
        exit(1);
    }
    //set the last character to a null terminating character
    buffer[stringMessage]='\0';
    
	
	off_t offsetBytesReceived;
	int messageBit;
	//check the data received from the NetRead request from client and see whether or not it returns -1, if so print error message
	if((messageBit=recv(packet->clientFileDescriptor,&offsetBytesReceived,sizeof(offsetBytesReceived),0))==-1){
        perror("ERROR: Netwrite request could not receive the offset from client");
    }
	//convert into 32-bit integer host byte order
	off_t numBytesToBeOffset =ntohl(offsetBytesReceived);

    int writeresult;
    pthread_mutex_lock(&userListMutex); 
        int writefd = open(newPath, O_WRONLY);
        
	packet->serverFileDescriptor = writefd;
	FileDescriptorTable* newPacket = (FileDescriptorTable*)malloc(sizeof(FileDescriptorTable));
	newPacket->packetData = packet;
	newPacket->next = NULL;
	insertLinkedList(newPacket);

	lseek(writefd, numBytesToBeOffset, SEEK_SET);
        if((writeresult = write(writefd, buffer, writenbyte)) < 0){
            perror("err writing to file: ");
        }else{
        }
    pthread_mutex_unlock(&userListMutex);

    int writeresultCurrent=htonl(writeresult);
    if(send(packet->clientFileDescriptor,&writeresultCurrent,sizeof(writeresultCurrent),0)==-1){
        perror("ERROR: NetWrite request was Unable to send result back to the client!\n");
    }

    //if there was an error getting the resulting size
    if(writeresult==-1){
        errorNumber=htonl(errno);
        if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1){
            perror("ERROR: NetWrite: Issue with sending errno");
        }
    }
    return packet;
}

clientPacketData* handleMkdirRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){
	

	int validPath=0;
    if((validPath=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){ 
        //getting file name to open
        perror("NetMkdir: Could not receive path");
        exit(1);
    }
	buffer[validPath]='\0';
	buffer = getFilename(buffer);   
    packet->fileName=malloc(sizeof(buffer));
	//copy the name of the filename into the buffer
    strcpy(packet->fileName, buffer);
	//check the flags received this means flags such as O_RDONLY,O_WRONLY,O_RDWR...etc
    int flagsReceived=0;
    int checkFlag=0;
	//if the flags received from the client side is equal to -1
	//then print error message saying we have an issue recing flags from client 
    if((checkFlag=recv(packet->clientFileDescriptor,&flagsReceived,sizeof(int),0))==-1){
        perror("ERROR: Netopen request has an issue in receiving flags!\n");
    }

	//Convert the flagsReceived into a 32-bit integer in host byte order 
    //this is used for data exchange with the method ntohl
    int flags=ntohl(flagsReceived);
	//initialize the packet field modeFlags with flags
    packet->modeFlags=flags;

	// try actually opening the dir and then sending the result FD back
    int result=0;
    result=mkdir(buffer,flags);
	//Check the result of open 
	//If not -1, then we know that the file was able to open successfully 
	//and we return the negative version of the server file descriptor back to the client side
    if(result!=-1){
      packet->serverFileDescriptor=-1*result;
    }

	//send over the data the server processed back to the server side!
	//first check to see if send does not return -1
	int currentResult=0;
    currentResult=result;
	//if so then we have a bad file descriptor
    if (send(packet->clientFileDescriptor,&currentResult,sizeof(int),0)==-1){
        perror("ERROR: NetMkdir request has received a bad file descriptor!\n");
    }
    //if there was an error getting the resulting FD
    if(result==-1){
        //Could not send data back to the client properly so we send errno
        if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1){
            perror("NetMkdir: issue with sending errno");
        }
	}
       //check the count of the file descriptor and make sure its less than the length of the fdArray
       //and set the countFileDescriptor to be equal to the current file descriptor
	return packet;
}


clientPacketData* handleReaddirRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){

    int msgReciever = 0;
    int offsetReciever = 0;
    if((msgReciever =recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){
        	perror("ERROR: Netreaddir request could not receive the directory name");
    }else{
    }

    if((offsetReciever=recv(packet->clientFileDescriptor, &offsetReciever,sizeof(int),0))==-1){
        	perror("ERROR: Netreaddir request could not receive the offset");
    }else{
    }

    //get the proper directory name and run readdir
    char* newPath = getFilename(buffer);

	pthread_mutex_lock(&userDirMutex);
    DIR* dirp = searchList(head, newPath);
	pthread_mutex_unlock(&userDirMutex);
    if(dirp == NULL){
    }

    struct dirent* readDirRes;
    memset(buffer, '\0', MAXBUFFERSIZE);
    readDirRes = readdir(dirp);

    //send result of readdir to client
    while(readDirRes){
        strcat(buffer, readDirRes -> d_name);
        strcat(buffer, "\n");
        readDirRes = readdir(dirp);
    }

    
    //send size of buffer back to the client
      //if(send(packet->clientFileDescriptor, buffer,sizeof(buffer),0)==-1)
    int bufferLen = strlen(buffer);
    if(send(packet->clientFileDescriptor, &bufferLen,sizeof(int),0)==-1){
       	 perror("ERROR: NetRead request has an issue in sending size result!");
    }
      
	//check whether or not the output buffer is equal to the null terminating character
	if(strcmp(buffer,"\0")==0){
	    	if(send(packet->clientFileDescriptor,buffer,bufferLen,0)==-1){
			    perror("ERROR: NetRead request broke while sending the buffer back to the client");
          	}
        
    }else{
        if(send(packet->clientFileDescriptor,buffer,bufferLen,0)==-1){
		    perror("ERROR: NetRead request broke while sending the buffer back to the client");
        }
    }
    return packet;
}




clientPacketData* handleReleasedirRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){

    int msgReciever = 0;
    if((msgReciever =recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){
        	perror("ERROR: releasedir request could not receive the directory name");
    }else{
    }

    //get the correct directory and run opendir
    char* newPath = (char*)malloc(sizeof(char) * 100);
    strcpy(newPath, MOUNTPATH);
    strcat(newPath, buffer); 

    //get the DIR* from LL and release it
	pthread_mutex_lock(&userDirMutex);
    DIR* closeMe = searchList(head, newPath);
	pthread_mutex_unlock(&userDirMutex);
    int closeResult = 0;
    if((closeResult = closedir(closeMe)) == -1){
        perror("releasedir: err =>");
    }else{
    }

    //remove info from linked list
	pthread_mutex_lock(&userDirMutex);
    deleteFromList(head, newPath);
	pthread_mutex_unlock(&userDirMutex);

    //send back errno, if necessary. otherwise send back 0
    int result = 0;
    if(closeResult != 0){
        result = errno;
    }
 	if(send(packet->clientFileDescriptor,&result,sizeof(int),0)==-1){
        printf("releasedir: failed to send err code to client\n"); 
        perror("error => ");
    }else{
    }

    return packet;   

}

clientPacketData* handleOpendirRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){
 
    int msgReciever = 0;
    if((msgReciever =recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){
        	perror("ERROR: open request could not receive the directory name");
    }else{
    }

    //get the correct directory and run opendir
    char* newPath = (char*)malloc(sizeof(char) * 100);
    strcpy(newPath, MOUNTPATH);
    strcat(newPath, buffer); 
    DIR* dirp = opendir(newPath);

    //insert the information into linked list
	pthread_mutex_lock(&userDirMutex);
    insertIntoList(head, newPath, dirp);
	pthread_mutex_unlock(&userDirMutex);

    //send back errno, if necessary. otherwise send back 0
    int result = 0;
    if(dirp == NULL){
        result = errno;
    }

 	if(send(packet->clientFileDescriptor,&result,sizeof(int),0)==-1){
        printf("opendir: failed to send err code to client\n"); 
        perror("error => ");
    }else{
    }

    return packet;   

}

clientPacketData* handleFlushRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){
    
    //get path name from client
    int msgReciever = 0;
    int flush_fd = 0;
    int fd = 0;
    if((msgReciever =recv(packet->clientFileDescriptor,&flush_fd,sizeof(int),0))==-1){
        	perror("ERROR: Netread request could not receive the directory name");
    }else{
	    fd = ntohl(flush_fd);
    }

    int result = fsync(fd);
    int message = htonl(result);
    if(send(packet->clientFileDescriptor,&message,sizeof(int),0) == -1){
	perror("ERROR: NetFlush request could not send the result\n");
    }
    if(result == -1){
	int errno_send = htonl(errno);
	if(send(packet->clientFileDescriptor,&errno_send,sizeof(int),0) == -1){
		perror("ERROR: NetFlush request could ont send the result\n");
	}
    }
    return packet;
}

clientPacketData* handleTruncateRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){
    
    //get path name from client
    int msgReciever = 0;
    int offset = 0;
    if((msgReciever =recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){
        	perror("ERROR: truncate request could not receive the directory name");
    }else{
    }

    //get offset from client
    if(recv(packet->clientFileDescriptor, &offset,sizeof(int),0) == -1){
        	perror("ERROR: truncate request could not receive the offset");
    }else{
    }

    //get actual path name
    char* newPath = getFilename(buffer);
    
    //convert offset using ntohl
    int actualOffset = ntohl(offset);
    
    //perform a truncation
    int result = 0;
    if((result = truncate(newPath, actualOffset))==-1){
        perror("error =>");
    }else{
    }

    //send back result
 	if(send(packet->clientFileDescriptor,&result,sizeof(int),0)==-1){
        printf("truncate: failed to send err code to client\n"); 
        perror("error => ");
    }else{
    }


    return packet;
}

clientPacketData* handleGetattrRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){

	struct stat* temp = (struct stat*)malloc(sizeof(struct stat));	
	int validPath=0;
    if((validPath=recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0))==-1){ 
	//getting file name to open
        perror("NetGetattr: Could not receive path");
        exit(1);
    }
	buffer[validPath]='\0';
	//If valid path them simply print the path to the file

    //concat the recieved string with the directory we're reading from 
    char* newPath = getFilename(buffer);

	int stat_result = stat(newPath, temp);
    if(S_ISREG(temp -> st_mode )){
    }else if (S_ISDIR(temp -> st_mode)){
    }else{
    }

	//send back the stat result back to the client
	if(send(packet->clientFileDescriptor, &stat_result,sizeof(int),0)==-1)
	{
		perror("ERROR: NetGetattr request has an issue in sending result!");
	}
	//if there was an error getting the resulting size
	if(stat_result == -1)
	{
		//send errno back to the client 
        	errorNumber=errno;
		//Check whether or not the data to be sent back to the client is equal to -1
        	if(send(packet->clientFileDescriptor,&errorNumber,sizeof(errorNumber),0)==-1)
		{
			perror("ERROR: NetRead request could not send errno to client!");
        	}
        	return packet;
	}
	else
	{
	        if(send(packet->clientFileDescriptor,temp,sizeof(struct stat),0)==-1)
		{
			perror("ERROR: NetRead request has an issue with sending the buffer back to the client");
	        }
	}
	return packet;

}

clientPacketData* handleReleaseRequest(clientPacketData* packet, char buffer[MAXBUFFERSIZE], int errorNumber){

	int msgReciever = 0;
	if((msgReciever = recv(packet->clientFileDescriptor,buffer,MAXBUFFERSIZE,0)) == -1){
		perror("ERROR: Release request could not receive the directory name");
	}else{
		buffer[msgReciever] = '\0';
	}

	FileDescriptorTable* currentFDTable = tableFD;
	FileDescriptorTable* previousFDTable = NULL;
	//for sync purposed
	pthread_mutex_lock(&userListMutex);
	int fd = 0;
	while(currentFDTable != NULL){
		//when the node to be deleted is equal to the same as the current node we are at then delete
		if(strcmp(currentFDTable->packetData->fileName, buffer) == 0)
		{
			//if there is only one node in the LL
			if(currentFDTable->next == NULL)
			{
				//check to see if the previousFDTable is null, if so the we conclude that there is only one node in the LL
				if(previousFDTable == NULL)
				{

					if((fd = currentFDTable->packetData->serverFileDescriptor)){
						close(fd);
					}
					tableFD = NULL;
					break;
				}
				//if the current file descriptor node is at the end of the linked list then set some stuff
				if((fd = currentFDTable->packetData->serverFileDescriptor)){
					close(fd);
				}
				previousFDTable->next = NULL;
				break;
			}
			//if more than one node and the node to be deleted is the head, then simply set tableFd to the next one
			if(previousFDTable == NULL)
			{
				if((fd = currentFDTable->packetData->serverFileDescriptor)){
					close(fd);
				}
				tableFD = tableFD->next;
				currentFDTable = tableFD;
				continue;
			}
			//Set previousFDTable->next to the node to be deleted next value which is currentFDTable->next
			
			if((fd = currentFDTable->packetData->serverFileDescriptor)){
				close(fd);
			}
			previousFDTable->next = currentFDTable->next;
			currentFDTable = previousFDTable->next;
			continue;
		}
		//increment the previousFDTable
		//and currentFDTable
		if(previousFDTable == NULL)
		{
			previousFDTable = currentFDTable;
			currentFDTable = currentFDTable->next;
			continue;
		}
		
		previousFDTable = previousFDTable->next;
		currentFDTable = currentFDTable->next;
	}
	//unlock for sync
	pthread_mutex_unlock(&userListMutex);

	return packet;

}

//function pointer for thread handler 
//this is used to get the message from the client side on whether they called 
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
		case NETREAD: //done
			printf("NetRead Request: IP Address %s\n",packet->ipAddress);
      			packet=handleReadRequest(packet, buffer, errorNumber);
      			printf("NetRead: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETWRITE: //done
			printf("NetWrite Request: IP Address %s\n",packet->ipAddress);
      			packet = handleWriteRequest(packet, buffer, errorNumber);
      			printf("NetWrite: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETCLOSE:
      			break;
		case NETCREATE: //done?
			printf("NetCreate Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleCreateRequest(packet, buffer, errorNumber);
      			printf("NetCreate: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETFLUSH:
			printf("NetFlush Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleFlushRequest(packet, buffer, errorNumber);
      			printf("NetFlush: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETRELEASE: 
			printf("NetRelease Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleReleaseRequest(packet, buffer, errorNumber);
      			printf("NetRelease: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETTRUNCATE: //maybe done?
			printf("NetTruncate Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleTruncateRequest(packet, buffer, errorNumber);
      			printf("NetTruncate: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETGETATTR: //done
			printf("NetGetattr Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleGetattrRequest(packet, buffer, errorNumber);
      			printf("NetGetattr: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETOPENDIR: //(I am not sure if this is necessary)
			printf("NetOpendir Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleOpendirRequest(packet, buffer, errorNumber);
      			printf("NetOpendir: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETREADDIR: //done
			printf("NetReaddir Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleReaddirRequest(packet, buffer, errorNumber);
      			printf("NetReaddir: Finished Operation.\n");
      			close(packet->clientFileDescriptor);
      			break;
		case NETRELEASEDIR:
			printf("NetReleasedir Requst: IP Address %s\n",packet->ipAddress);
      			packet=handleReleasedirRequest(packet, buffer, errorNumber);
      			printf("NetReleasedir: Finished Operation.\n");
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
int main(int argc, char * argv[]){

    //initialize the directory linked list
    head = (dirNode*)malloc(sizeof(dirNode));
    head->next = NULL;

    //parse flags and set !
    //assume that in order to run te server, the order of the parameters is:
    //  ./serverSNFS --port 12345 --mount /tmp/Some_Dir
    
    PORT = argv[2]; 
    MOUNTPATH = argv[4]; 

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
    		if((msg=recv(serverFileDescriptor,&msg_type,sizeof(int),0))==-1){
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
