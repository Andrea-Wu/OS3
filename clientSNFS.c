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
#include <dirent.h>

struct addrinfo clientSideData;
struct addrinfo* serverSideData;
char ipAddressArray[INET_ADDRSTRLEN];
/*
void loggingFnStrAndrea(char* string){
    //this is shit and it only writes a string
    //make a copy of this that also writes ints
    FILE* fp = fopen("/ilab/users/ayw19/Workspace/OS3/", "a");
    fprintf(fp,"%s\n", string);
    fclose(fp);
}


void loggingFnIntAndrea(int myInt){
    FILE* fp = fopen("/ilab/users/ayw19/Workspace/OS3/", "a");
    fprintf(fp,"%d\n", myInt);
    fclose(fp);
}
*/
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


int do_getattr(const char* path, struct stat* st){

    printf("fuse intercepts getattr\n");
	int sockDescriptor=-1;
	//Establish a connection for the NetGetattr requested
    sockDescriptor=connectionForClientRequests(sockDescriptor);
    printf("Netgetattr: Connected to %s\n", ipAddressArray);
	//For Thread synchronization
	
	
	int netreadMessage=htonl(NETGETATTR);
	//send data to server side with message being NetRead
    if(send(sockDescriptor,&netreadMessage,sizeof(int),0)==-1){
        perror("NetRead request fails");
    }
	
	printf("NetGetattr: Sending Path.\n");
    if(send(sockDescriptor,path,strlen(path),0)==-1){
        perror("ERROR: NetGetattr request could not send path!\n");
    }
    
	int result=0;
    int resultMessage=0;
    if((resultMessage=recv(sockDescriptor,&result,sizeof(result),0))==-1){
        perror("ERROR: NetGetattr request could not read receive the results");
    }
	
	//Check for possible errors from NetGetattr requests
    if(result==-1){
        int errorMessage=0;
        if((resultMessage=recv(sockDescriptor,&errorMessage,sizeof(errorMessage),0))==-1){
            perror("NetGetattr requests receives an error");
        }
        errno=errorMessage;
        //set the value of errno
        printf("Errno Is: %s\n",strerror(errno));
    }else{
        //char resultString[MAXBUFFERSIZE];
        if(recv(sockDescriptor,st,sizeof(struct stat),0)==-1){
            perror("NetGetattr requests receives error from server");
        }
    }
	
    close(sockDescriptor);
	return result;
}

//create
int do_create(const char * path, mode_t mode, struct fuse_file_info * ffi){
    printf("fuse intercepts create\n");
	
	int sockDescriptor=-1;
	//estbalish a connection for the following socket descriptor
    sockDescriptor=connectionForClientRequests(sockDescriptor);
    printf("NetCreate: Connected to %s\n",ipAddressArray);
   	
    /***Make a seperate method for sending pathname and flags from the code below!***/
	printf("NetCreate: Sending File Mode.\n");
	
	int createRequest=htonl(NETOPEN);
    if(send(sockDescriptor,&createRequest,sizeof(createRequest),0)==-1){
        perror("ERROR: NetCreate request could not read the message!\n");
    }
        
    if(send(sockDescriptor,path,strlen(path),0)==-1){
        perror("ERROR: NetCreate request could not send path!\n");
    }else{
        printf("NetCreate sent path to server\n");
    }
        
	//sending flags to server side to handle client requests
    int flagsForRequest=htonl(mode);
    if(send(sockDescriptor,&flagsForRequest,sizeof(flagsForRequest),0)==-1){
        perror("ERROR: NetCreate request could not send flags to server!");
    }else{
        printf("NetCreate: sent flags to server\n");
    }


    //printf("NetCreate: waiting to receive result.\n");
    int resultFileDescriptor=0;
    int resultMsg=0;
	//Receiving data back from the server side, check whether or not server side returns an error
    if((resultMsg=recv(sockDescriptor,&resultFileDescriptor,sizeof(resultFileDescriptor),0))==-1){
        perror("ERROR: NetCreate request could not receive result from server!\n");
        exit(-1);
    }
    printf("NetCreate: recieved result FD!\n");
    int resultCode=resultFileDescriptor;
    int errorTypeMessage=0;
	//Now that we received a file descriptr make sure it does not equal -1,
    //which means there is an error!
	if(resultCode==-1){
		//NetCreate fails to receive errno from server
        if((resultCode=recv(sockDescriptor,&errorTypeMessage,sizeof(errorTypeMessage),0))==-1){
            perror("ERROR: NetCreate request does not receive errno!\n");
        }
		//Checking errno and whether permission was denied!
        if(errorTypeMessage==DENIED_ACCESS){
            errno=13;
            perror("ERROR: NetCreate request could not open file due to restrictions.");
        }else{
            errno=errorTypeMessage;
            perror("ERROR from server side!");
        }
    }
	//close the socket
    close(sockDescriptor);
	//return the result
        
	ffi->fh = resultCode;

    return 0;
}

//open
int do_open(const char * path , struct fuse_file_info * ffi){
    	printf("open => path is %s\n", path);
	
	int sockDescriptor=-1;
	//estbalish a connection for the following socket descriptor
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("NetOpen: Connected to %s\n",ipAddressArray);
   	
    	/***Make a seperate method for sending pathname and flags from the code below!***/
	printf("NetOpen: Sending File Mode.\n");
	int netOpenRequestResults=0;
	netOpenRequestResults=handleNetOpenRequests(sockDescriptor,path,ffi->flags);	
    printf("returned from hnor\n");
    if(netOpenRequestResults < 0){
		ffi->fh = netOpenRequestResults;
	}
    return 0;
}

int handleNetOpenRequests(int sockDescriptor,const char* pathname,int flags){
    int openRequest=htonl(NETOPEN);
    if(send(sockDescriptor,&openRequest,sizeof(openRequest),0)==-1){
        perror("ERROR: NetOpen request could not read the message!\n");
    }
    sleep(1);

    if(send(sockDescriptor,pathname,strlen(pathname),0)==-1){
        perror("ERROR: NetOpen request could not send path!\n");
    }else{
        printf("NetOpen: sent path.\n");
    }
    sleep(1);

	//sending flags to server side to handle client requests
    int flagsForRequest=htonl(flags);
    if(send(sockDescriptor,&flagsForRequest,sizeof(flagsForRequest),0)==-1){
        perror("ERROR: NetOpen request could not send flags to server!");
    }else{
        printf("NetOpen: Sending Flags\n");
    }
    sleep(1);

    //printf("NetOpen: waiting to receive result.\n");
    int resultFileDescriptor=0;
    int resultMsg=0;
    //Receiving data back from the server side, check whether or not server side returns an error
    if((resultMsg=recv(sockDescriptor,&resultFileDescriptor,sizeof(resultFileDescriptor),0))==-1){
        perror("ERROR: NetOpen request could not receive result from server!\n");
        exit(-1);
    }else{ 
        printf("NetOpen: recieved result FD!\n");
    }
    sleep(1);

    int resultCode=resultFileDescriptor;
    int errorTypeMessage=0;
	//Now that we received a file descriptr make sure it does not equal -1, 
    //which means there is an error!
	if(resultCode==-1){
        //NetOpen fails to receive errno from server
        if((resultCode=recv(sockDescriptor,&errorTypeMessage,sizeof(errorTypeMessage),0))==-1){
                 perror("ERROR: NetOpen request does not receive errno!\n");
        }
		//Checking errno and whether permission was denied!
        if(errorTypeMessage==DENIED_ACCESS){
            errno=13;
            perror("ERROR: NetOpen request could not open file due to restrictions.");
        }else{
            errno=errorTypeMessage;
            perror("ERROR from server side!");
        }
    }
	//close the socket
        close(sockDescriptor);
	//return the result
        if(resultCode==-1)
        {
                 return resultCode;
        }
	
        return -1*resultCode;
	
}

int do_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* ffi){
	int sockDescriptor=-1;
	//Establish a connection for the NetRead request
    sockDescriptor=connectionForClientRequests(sockDescriptor);
    printf("read: Connected to %s\n", ipAddressArray);
	//For Thread synchronization
    sleep(1);


    //tell server what type of request to process
    int netreadMessage=htonl(NETREAD);
    if(send(sockDescriptor,&netreadMessage,sizeof(int),0)==-1){
        perror("Read fails to send request type");
    }else{
        printf("read sent request type\n");
    }

    sleep(1);
	printf("Read: Sending Directory path name\n");
    //Send String to the client 
    if(send(sockDescriptor,path,strlen(path),0)==-1){
        perror("ERROR: read request fails to send directory path name to the server!\n");
    }
    
	sleep(1);
	int offset_to_read = htonl(offset);
	if(send(sockDescriptor,&offset_to_read, sizeof(int),0) == -1){
		perror("ERROR: read cound not send the offset to server\n");
	}

    sleep(1);
	//Here we set up to receive the result from the server
	printf("read: waiting to receive result\n");
    int resultSize=0;
    int resultMessage=0;
    if((resultMessage=recv(sockDescriptor,&resultSize,sizeof(resultSize),0))==-1){
        perror("read request could not receive result size");
    }else{
        printf("read: Received result size: %d\n", resultSize);
    }
	int result=resultSize;
    //DECLARE BUF

    //char* buf = (char*)malloc(sizeof(char) * MAXBUFFERSIZE);

    //Check for possible errors from NetRead requests
    if(result==-1){
        int errorMessage=0;
        if((resultMessage=recv(sockDescriptor,&errorMessage,sizeof(errorMessage),0))==-1){
            perror("read requests receives an error");
        }
        errno=errorMessage;
        //set the value of errno
        printf("Errno Is: %s\n",strerror(errno));
    }else{
        if(recv(sockDescriptor,buf,resultSize,0)==-1){
            perror("read requests receives error from server");
        }else{
            printf("recieved from server=> %s\n", buf);
        }

    }
	    
    close(sockDescriptor);
//Read should return exactly the number of bytes requested except on EOF or error, otherwise the rest of the data will be substituted with zeroes - FROM DOCS
    return strlen(buf);
}

//write
int do_write(const char * path, const char * string, size_t size, off_t offset ,struct fuse_file_info * ffi){
    //i think the 1st string is the path 
    //printf("writed => path is %s, string is %s, size is %d, offset is %d\n", path,string,size,offset);
    
	int sockDescriptor=-1;
	//establish a connection for the NetWrite Requst
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("netwrite: Connected to %s\n",ipAddressArray);
	
	//convert to a 32-bit integer host byte order and send to server
    	int netRequest=htonl(NETWRITE);
    	if(send(sockDescriptor,&netRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send message to server!\n");
    	}
	
	printf("NetWrite request sending over file descriptor to server\n");
    	int filedescriptorWriteRequest=htonl(ffi->fh);
	//Sending over the data to the client
    	if(send(sockDescriptor,&filedescriptorWriteRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over the file descriptor to the server!\n");
    	}

    	
	//sending byte size to the client
    	//printf("netwrite: Sending nbyte to Server: %d\n", size);
    	int sizeOfRequest=htonl(size);
    	if(send(sockDescriptor,&sizeOfRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over the bytes to the server!\n");
    	}
	
	//Send String to the client 
    	//printf("netwrite: Sending String.\n");
    	if(send(sockDescriptor,string,strlen(string),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over string message to the server!\n");
    	}
	
	//Send offset to the client 
    	int offsetOfRequest=htonl(offset);
    	if(send(sockDescriptor,&offsetOfRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetWrite request fails to send over string message to the server!\n");
    	}


    	int resultSize=0;
    	int resultMessage=0;
    	if((resultMessage=recv(sockDescriptor,&resultSize,sizeof(resultSize),0))==-1)
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
		if((resultMessage=recv(sockDescriptor,&errorResult,sizeof(errorResult),0))==-1)
		{
            		perror("netwrite received an error!");
        	}
		errno=ntohl(errorResult);
		perror("NetWrite request received errno!\n");
	}
	//close the scoket descriptor
    	close(sockDescriptor);
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
    /*
    printf("openDired -> path is %s\n", path);

    //Establish connection for netOpendir request
    sockDescriptor=connectionForClientRequests(sockDescriptor);
    printf("opendir: Connected to %s\n", ipAddressArray);
    sleep(1);	

    //tell server what type of request to process
    int netreadMessage=htonl(NETREADDIR);
    if(send(sockDescriptor,&netreadMessage,sizeof(int),0)==-1){
        perror("opendir request fails");
    }else{
        printf("opendir sent file mode\n");
    }
    sleep(1);

    //send directory path name to server
    if(send(sockDescriptor,path,strlen(path),0)==-1){
        perror("ERROR: opendir request fails to send over string message to the server!\n");
    }else{
        printf("opendir: sent directory path name\n");
    }
    
    //recieve a "DIR" structure from server
    DIR* resultDirp
    if((resultMessage=recv(sockDescriptor,&resultDirp,sizeof(DIR),0))==-1){
        perror("ERROR: Netopenddir request could not receive directory stream");
    }else{
        printf("Netopenddir: Received directory stream ");
    }
*/
    printf("does opendir do anything\n");
    return 0;
}

//readdir
int do_readdir(const char * path, void * buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * ffi){
    int sockDescriptor=-1;
	//Establish a connection for the NetRead request
    sockDescriptor=connectionForClientRequests(sockDescriptor);
    printf("readdir: Connected to %s\n", ipAddressArray);
	//For Thread synchronization
    sleep(1);


    //tell server what type of request to process
    int netreadMessage=htonl(NETREADDIR);
    if(send(sockDescriptor,&netreadMessage,sizeof(int),0)==-1){
        perror("Readdir fails to send request type");
    }else{
        printf("readdir sent request type\n");
    }

    sleep(1);
	printf("ReadDir: Sending Directory path name\n");
    //Send String to the client 
    if(send(sockDescriptor,path,strlen(path),0)==-1){
        perror("ERROR: readdir request fails to send directory path name to the server!\n");
    }
    
	sleep(1);
	int offset_to_read = htonl(offset);
	if(send(sockDescriptor,&offset_to_read, sizeof(int),0) == -1){
		perror("ERROR: readdir cound not send the offset to server\n");
	}

    sleep(1);
	//Here we set up to receive the result from the server
	printf("NetReaddir: waiting to receive result\n");
    int resultSize=0;
    int resultMessage=0;
    if((resultMessage=recv(sockDescriptor,&resultSize,sizeof(resultSize),0))==-1){
        perror("ERROR: NetReaddir request could not receive result size");
    }else{
        printf("NetReaddir: Received result size: %d\n", resultSize);
    }
	int result=resultSize;
//DECLARE BUF

     //   char* buf = (char*)malloc(sizeof(char) * MAXBUFFERSIZE);

    //Check for possible errors from NetRead requests
    if(result==-1){
        int errorMessage=0;
        if((resultMessage=recv(sockDescriptor,&errorMessage,sizeof(errorMessage),0))==-1){
            perror("NetRead requests receives an error");
        }
        errno=errorMessage;
        //set the value of errno
        printf("Errno Is: %s\n",strerror(errno));
    }else{
        char buf[MAXBUFFERSIZE];
        if(recv(sockDescriptor,buf,resultSize,0)==-1){
            perror("NetRead requests receives error from server");
        }else{
            printf("recieved from server=> %s\n", buf);
            printf("result from tokenizng buffer:\n");
            char* token = strtok(buf, "\n");
            while(token){
                printf("%s\n", token);
                filler(buffer, token, NULL, 0);
                token = strtok(NULL, "\n");
                printf("---");
            }

        }

    }
	    
    	close(sockDescriptor);
    	return 0;


}

//releasedir
int do_releasedir(const char * path, struct fuse_file_info * ffi){
    printf("releaseDired\n");
    return 0;
}

//mkdir
int do_mkdir(const char * path, mode_t mode){
    printf("mkdired\n");

	int sockDescriptor=-1;
	//establish a connection for the NetWrite Requst
    	sockDescriptor=connectionForClientRequests(sockDescriptor);
    	printf("netmkdir: Connected to %s\n",ipAddressArray);
	
	//convert to a 32-bit integer host byte order and send to server
    	int netRequest=htonl(NETMKDIR);
    	if(send(sockDescriptor,&netRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetMkdir request fails to send message to server!\n");
    	}
	
	//Send path to the client 
    	if(send(sockDescriptor,path,strlen(path),0)==-1)
	{
        	perror("ERROR: NetMkdir request fails to send over string message to the server!\n");
    	}
	
	int modeOfRequest=htonl(mode);
    	if(send(sockDescriptor,&modeOfRequest,sizeof(int),0)==-1)
	{
        	perror("ERROR: NetMkdir request fails to send over the bytes to the server!\n");
    	}
	
	
	int resultSize=0;
    	int resultMessage=0;
    	if((resultMessage=recv(sockDescriptor,&resultSize,sizeof(resultSize),0))==-1)
	{
        	perror("ERROR: NetMkdir request could not receive results from the server!\n");
    	}
	int result=ntohl(resultSize);
    	printf("NetMkdir: Received result size of: %d\n", result);
	//check result and see if any errors
    	if(result==-1)
	{
        	int errorResult;
		if((resultMessage=recv(sockDescriptor,&errorResult,sizeof(errorResult),0))==-1)
		{
            		perror("netmkdir received an error!");
        	}
		errno=ntohl(errorResult);
		perror("NetMkdir request received errno!\n");
	}
	//close the scoket descriptor
    	close(sockDescriptor);

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

