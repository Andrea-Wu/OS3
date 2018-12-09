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

	netserverinit("facade.cs.rutgers.edu");
	int fd = netopen("./thing.txt", O_RDWR);
	
}
