#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "header.h"
#include "libnetfiles.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

struct addrinfo clientSideData;
struct addrinfo* serverSideData;
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


int do_getattr(const char* path, struct stat* st, struct fuse_file_info* ffi){

        printf("fuse intercepts getattr\n");
	int sockDescriptor=-1;
	//Establish a connection for the NetGetattr requested
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("Netgetattr: Connected to %s\n", ipAddressArray);
	//For Thread synchronization
	sleep(1);
	
	int netreadMessage=htonl(NETGETATTR);
	//send data to server side with message being NetRead
    	if(send(sockDescriptor,&netreadMessage,sizeof(int),0)==-1)
	{
        	perror("NetRead request fails");
    	}
	sleep(1);
	printf("NetGetattr: Sending Path.\n");
        if(send(sockDescriptor,path,strlen(path),0)==-1)
        {
                perror("ERROR: NetGetattr request could not send path!\n");
        }
        sleep(1);

	


/////////////////////
	int result=0;
    	int resultMessage=0;
    	if((resultMessage=recv(sockDescriptor,&result,sizeof(result),0))==-1)
	{
        	perror("ERROR: NetGetattr request could not read receive the results");
    	}
	
	//Check for possible errors from NetGetattr requests
    	if(result==-1)
	{
        	int errorMessage=0;
		if((resultMessage=recv(sockDescriptor,&errorMessage,sizeof(errorMessage),0))==-1)
		{
            		perror("NetGetattr requests receives an error");
        	}
		errno=errorMessage;
		//set the value of errno
        	printf("Errno Is: %s\n",strerror(errno));
    	}
    	else
	{
        	//char resultString[MAXBUFFERSIZE];
        	if(recv(sockDescriptor,st,sizeof(struct stat),0)==-1)
		{
            		perror("NetGetattr requests receives error from server");
        	}
    	}
	
    	close(sockDescriptor);
	
        return result;
}

//create
int do_create(const char * path, mode_t mode, struct fuse_file_info * ffi){
    printf("fuse intercepts create\n");
	
	int socketDescriptor=-1;
	//estbalish a connection for the following socket descriptor
    	socketDescriptor=connectionForClientRequests(socketDescriptor);
    	printf("NetCreate: Connected to %s\n",ipAddressArray);
   	sleep(1);
    	/***Make a seperate method for sending pathname and flags from the code below!***/
	printf("NetCreate: Sending File Mode.\n");
	
	int createRequest=htonl(NETOPEN);
        if(send(socketDescriptor,&createRequest,sizeof(createRequest),0)==-1)
        {
                perror("ERROR: NetCreate request could not read the message!\n");
        }
        sleep(1);
	printf("NetCreate: Sending Path.\n");
        if(send(socketDescriptor,path,strlen(path),0)==-1)
        {
                perror("ERROR: NetCreate request could not send path!\n");
        }
        sleep(1);
	//sending flags to server side to handle client requests
        printf("NetCreate: Sending Flags\n");
        int flagsForRequest=htonl(mode);
        if(send(socketDescriptor,&flagsForRequest,sizeof(flagsForRequest),0)==-1)
        {
                perror("ERROR: NetCreate request could not send flags to server!");
        }
        //printf("NetCreate: waiting to receive result.\n");
        int resultFileDescriptor=0;
        int resultMessageFromServer=0;
	//Receiving data back from the server side, check whether or not server side returns an error
        if((resultMessageFromServer=recv(socketDescriptor,&resultFileDescriptor,sizeof(resultFileDescriptor),0))==-1)
        {
                perror("ERROR: NetCreate request could not receive result from server!\n");
                exit(-1);
        }
        printf("NetCreate: recieved result FD!\n");
        int resultFromServer=resultFileDescriptor;
        int errorTypeMessage=0;
	//Now that we received a file descriptr make sure it does not equal -1, which means there is an error!
	if(resultFromServer==-1)
        {
		//NetCreate fails to receive errno from server
                if((resultFromServer=recv(socketDescriptor,&errorTypeMessage,sizeof(errorTypeMessage),0))==-1)
                {
                         perror("ERROR: NetCreate request does not receive errno!\n");
                }
		//Checking errno and whether permission was denied!
                if(errorTypeMessage==DENIED_ACCESS)
                {
                        errno=13;
                        perror("ERROR: NetCreate request could not open file due to restrictions.");
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
        
	ffi->fh = resultFromServer;

    return 0;
}

//open
int do_open(const char * path , struct fuse_file_info * ffi){
    	printf("open => path is %s\n", path);
	
	int sockDescriptor=-1;
	//estbalish a connection for the following socket descriptor
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("NetOpen: Connected to %s\n",ipAddressArray);
   	sleep(1);
    	/***Make a seperate method for sending pathname and flags from the code below!***/
	printf("NetOpen: Sending File Mode.\n");
	int netOpenRequestResults=0;
	netOpenRequestResults=handleNetOpenRequests(sockDescriptor,path,ffi->flags);
	if(netOpenRequestResults < 0){
		ffi->fh = netOpenRequestResults;
	}

    	return 0;
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

int do_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* ffi){
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
    	int filedescriptorNetReadRequest=htonl(ffi->fh);
	//sending over the file descriptor to the server side in order to read the file
    	if(send(sockDescriptor,&filedescriptorNetReadRequest,sizeof(int),0)==-1)
	{
        	perror("NetRead request fails");
    	}
	sleep(1);
    	int sizeToRead=htonl(size);
    	if(send(sockDescriptor,&sizeToRead,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetRead could not send the bytes over to be read to the server");
    	}
	
	int offset_to_read = htonl(offset);
	if(send(sockDescriptor,&offset_to_read, sizeof(int),0) == -1){
		perror("ERROR: NetRead cound not send the bytes over to be read to the server");
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
        	if(recv(sockDescriptor,buf,size,0)==-1)
		{
            		perror("NetRead requests receives error from server");
        	}
    	}
	
    	close(sockDescriptor);
    	return result;

}

//write
int do_write(const char * path, const char * string, size_t size, off_t offset ,struct fuse_file_info * ffi){
    //i think the 1st string is the path 
    //printf("writed => path is %s, string is %s, size is %d, offset is %d\n", path,string,size,offset);
    
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
    	int filedescriptorWriteRequest=htonl(ffi->fh);
	//Sending over the data to the client
    	if(send(socketDescriptor,&filedescriptorWriteRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over the file descriptor to the server!\n");
    	}

    	sleep(1);
	//sending byte size to the client
    	//printf("netwrite: Sending nbyte to Server: %d\n", size);
    	int sizeOfRequest=htonl(size);
    	if(send(socketDescriptor,&sizeOfRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over the bytes to the server!\n");
    	}
	sleep(1);
	//Send String to the client 
    	//printf("netwrite: Sending String.\n");
    	if(send(socketDescriptor,string,strlen(string),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over string message to the server!\n");
    	}
	sleep(1);
	//Send offset to the client 
    	int offsetOfRequest=htonl(offset);
    	if(send(socketDescriptor,&offsetOfRequest,sizeof(int),0)==-1)
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

//close 
int do_flush(const char * path, struct fuse_file_info * ffi){
    printf("flushing => path is %s\n", path);
    return 0;
}

int do_release(const char * path, struct fuse_file_info * ffi){
    printf("released => path is %s\n", path);
    return 0;
}

//truncate
int do_truncate(const char * path, off_t offset){
    printf("truncated\n");
    return 0;
}

//opendir
int do_opendir(const char * path, struct fuse_file_info * ffi){

    printf("openDiredi -> path is %s\n", path);
    return 0;
}

//readdir
int do_readdir(const char * path, void * idk, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * ffi){
    	int sockDescriptor=-1;
	//Establish a connection for the NetRead requested
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("readdir: Connected to %s\n", ipAddressArray);
	//For Thread synchronization
	sleep(1);
	printf("readdir: Sending File Mode\n");
	
/*
	int messageOpenRequest=htonl(NETOPEN);
	//send data to server side with the message being NetOpen, since we must open the file before we read it
	if(send(sockDescriptor,&messageOpenRequest,sizeof(messageOpenRequest),0)==-1)
	{
        	perror("NetOpen request fails");
    	}
	sleep(1);
*/
    	int netreadMessage=htonl(NETREADDIR);
	//send data to server side with message being NetRead
    	if(send(sockDescriptor,&netreadMessage,sizeof(int),0)==-1){
        	perror("ReadDir request fails");
    	}

    	sleep(1);
	    printf("ReadDir: Sending Directory path name\n");
    	//Send String to the client 
    	//printf("netwrite: Sending String.\n");
    	if(send(socketDescriptor,path,strlen(path),0)==-1){
        	perror("ERROR: NetWrite request fails to send over string message to the server!\n");
    	}
    
	
	int offset_to_read = htonl(offset);
	if(send(sockDescriptor,&offset_to_read, sizeof(int),0) == -1){
		perror("ERROR: NetRead cound not send the bytes over to be read to the server");
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
        	if(recv(sockDescriptor,buf,size,0)==-1)
		{
            		perror("NetRead requests receives error from server");
        	}
    	}
	
    	close(sockDescriptor);
    	return result;


}

//releasedir
int do_releasedir(const char * path, struct fuse_file_info * ffi){
    printf("releaseDired\n");
    return 0;
}

//mkdir
int do_mkdir(const char * path, mode_t mode){
    printf("mkdired\n");

	int socketDescriptor=-1;
	//establish a connection for the NetWrite Requst
    	socketDescriptor=connectionForClientRequests(socketDescriptor);
    	printf("netmkdir: Connected to %s\n",ipAddressArray);
	sleep(1);
	//convert to a 32-bit integer host byte order and send to server
    	int netRequest=htonl(NETMKDIR);
    	if(send(socketDescriptor,&netRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetMkdir request fails to send message to server!\n");
    	}
	sleep(1);
	//Send path to the client 
    	if(send(socketDescriptor,path,strlen(path),0)==-1)
	{
        	perror("ERROR: NetMkdir request fails to send over string message to the server!\n");
    	}
	sleep(1);
	int modeOfRequest=htonl(mode);
    	if(send(socketDescriptor,&modeOfRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetMkdir request fails to send over the bytes to the server!\n");
    	}
	sleep(1);
	
	int resultSize=0;
    	int resultMessage=0;
    	if((resultMessage=recv(socketDescriptor,&resultSize,sizeof(resultSize),0))==-1)
	{
        	perror("ERROR: NetMkdir request could not receive results from the server!\n");
    	}
	int result=ntohl(resultSize);
    	printf("NetMkdir: Received result size of: %d\n", result);
	//check result and see if any errors
    	if(result==-1)
	{
        	int errorResult;
		if((resultMessage=recv(socketDescriptor,&errorResult,sizeof(errorResult),0))==-1)
		{
            		perror("netmkdir received an error!");
        	}
		errno=ntohl(errorResult);
		perror("NetMkdir request received errno!\n");
	}
	//close the scoket descriptor
    	close(socketDescriptor);

    return 0;
}


int main(int argc, char* argv[]){
    char* hostname = "facade.cs.rutgers.edu";
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

    struct fuse_operations* fops = (struct fuse_operations*)malloc(sizeof(struct fuse_operations));
    fops -> getattr = do_getattr;
    fops -> mkdir = do_mkdir;
    fops -> create = do_create;
    fops -> open = do_open;
    fops -> write = do_write;
    fops -> read = do_read;
    fops -> truncate = do_truncate;
    fops -> opendir = do_opendir;
    fops -> readdir = do_readdir;
    fops -> releasedir = do_releasedir;
    fops -> mkdir = do_mkdir;
    fops -> flush = do_flush;
    fops -> release = do_release;
    return fuse_main( argc, argv, fops, NULL );
}

