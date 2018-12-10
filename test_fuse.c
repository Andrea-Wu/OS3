#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(){

//    int aaa = open("/tmp/fuse/what_is_this??.txt", O_RDWR);
//    printf("not open success %d\n", aaa);
    printf("%d\n",creat("/tmp/fuse/bob.txt", O_WRONLY));
    printf("%d\n",creat("/tmp/fuse/help.txt", O_WRONLY));
   
    //writing bob.txt
   printf("did a write: code %d\n",write(3, "fuck", 100));
   perror("aaaa");

    char* buf = (char*)malloc(sizeof(char) * 10);
    read(3, buf, 10);
    printf("I read => %s\n",buf);

   //printf("did a write: code %d\n",write(4, "fuck", 100));
   //perror("bbbb");
    return 0;
}
