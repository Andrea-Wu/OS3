
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <arpa/inet.h>

#include "libnetfiles.h"

int main(int argc, const char* argv[]) {
	
	printf("\n\n\n\n\nCLIENT STARTED\n");
	
	printf("\tTESTING read-write\n");
	netserverinit("facade.cs.rutgers.edu");
	int fd = netopen("./thing.txt", O_RDWR);
	
	printf("*** The file descriptor is: %d\n", fd);
	char buffer[6];
	int n = netread(fd, buffer, 5);
	buffer[5] = '\0';
	printf("\tReturn val: %d\n", n);

	n = netwrite(fd, buffer, 5);
	printf("\tReturn val: %d\n", n);
	printf("\tERROR: %s\n", strerror(errno));
	
	
	printf("\tClosing fd: %d\n", fd);
	netclose(fd);
	printf("\tClosing fd: 1\n");
	netclose(1);
	printf("\tClosing fd: 0\n");
	netclose(0);
	printf("\tClosing fd: -17\n");
	netclose(-17);


	/*
	netserverinit("facade.cs.rutgers.edu", UNRESTRICTED);
	
	//printf("Text nonexist\n");
	int fd = netopen("./somedir", O_RDWR);
	close(fd);
	
	printf("\nTest write-only\n");
	fd = netopen("./readless.txt", O_RDONLY);
	char info[3];
	int n = netread(fd, info, 3);
	printf("%d, %d\n", errno, n);
	netclose(fd);
	
	printf("\nTest read-only\n");
	fd = netopen("./writeless.txt", O_RDWR);
	char* info2 = "abc";
	n = netwrite(fd, info2, 3);
	printf("%d, %d\n", errno, n);
	netclose(fd);
	
	printf("\nTest a bunch of stuff\n");
	printf("%d\n", EACCES);
	int r = netopen("./thing.txt", O_RDONLY);
	printf("r: %d\n", r);
	int rw = netopen("./thing.txt", O_RDWR);
	printf("rw: %d\n", rw);
	int w = netopen("./thing.txt", O_WRONLY);
	printf("w: %d\n", w);

	netclose(r);
	netclose(rw);
	netclose(w);
	*/
	
	return 0;
}
