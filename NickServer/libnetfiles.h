#define MAXBUFFERSIZE 512
#define PORT "11111"
#define INVALID_FILE_MODE -214
#define DENIED_ACCESS -215
#include <netinet/in.h>
#include <sys/stat.h>
/*
 *Function prototypes for libnetfiles library
*/
int netserverinit(char *hostname);
int netopen(const char *pathname, int flags);
ssize_t netread(int filedes, void *buf, size_t nbyte);
int connectMethod(int socketDescript);
ssize_t netwrite(int filedes, const void *buf, size_t nbyte);
int netclose(int fd);
/*
 * Commands for libnetfiles library NETOPEN=1, NETREAD=2, NETWRITE=3, NETCLOSE=4
*/ 
#define NETOPEN 1
#define NETREAD 2
#define NETWRITE 3
#define NETCLOSE 4
#define NETCREATE 5
#define NETFLUSH 6
#define NETRELEASE 7
#define NETTRUNCATE 8
#define NETGETATTR 9
#define NETOPENDIR 10
#define NETREADDIR 11
#define NETRELEASEDIR 12
#define NETMKDIR 13

typedef struct packet
{  
  char ipAddress[INET_ADDRSTRLEN]; //character of the ipAddress for the network with INET_ADDRSTRLEN referring too the length of the string form for the IP Address
  int clientFileDescriptor; //file descriptor on the client side
  int messageMode; // associated with netread netwrite netopen netclose etc
  int serverFileDescriptor; //file descriptor used on the server side
  int modeFlags; //this is associated with O_RD, O_WR,O_RDONLY, etc
  char* fileName;
} clientPacketData;

/*
 *File Descriptor Table for all the requests sent by the client to the server and stored into the linked list 
*/
typedef struct table 
{
    clientPacketData *packetData;
    struct table *next;

}FileDescriptorTable;

//FileDescriptorTable* tableFD=NULL;
