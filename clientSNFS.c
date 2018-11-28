#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <header.h>

static struct fuse_operations = {
    .getattr = do_getattr,
    .create = do_create
}

int main(int argc, char* argv[]){
    

}

int do_getattr(const char* path, struct stat* st){
        st-> st_uid = getuid();
        st->st_gid = getgid();
        st->st_atime = time( NULL );
        st->st_mtime = time( NULL ); 

        if(strcmp(path, "/") == 0){ //what is "/" relative to?
            st->st_mode = S_IFDIR | 0755;
            st->st_nlink = 2; 
        }else{
            st->st_mode = S_IFREG | 0644;
            st->st_nlink = 1;
            st->st_size = 1024;
        }
        return 0
}
