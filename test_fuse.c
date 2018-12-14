#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(){

    opendir("/tmp/fuse/fake_dir");
    perror("opendir err =>");   
  


    readdir(opendir("/tmp/fuse/asss"));

    perror("some err =>");
    /*

    readdir(opendir("/tmp/fuse"));
    perror("readdir err: ");

    */
//creat("/tmp/fuse/newFile",S_IRUSR);
/*
   int aaa = open("/tmp/fuse/.newFile.txt", O_CREAT | O_RDWR);
   printf("fd is %d", aaa);
  */
/*
  //int trunc = truncate("/tmp/Andrea/hello.txt", 10);
int trunc = truncate("/tmp/fuse/ass2.txt", 10);

  printf("trunc result is %d\n",trunc );
  perror("err =>");
*/
/*
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
    */
}
