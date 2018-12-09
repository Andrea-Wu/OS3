#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "header.h"

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

        printf("fuse intercepts getattr\n");
        return 0;
}

//create
int do_create(const char * path, mode_t mode, struct fuse_file_info * ffi){
    printf("fuse intercepts create\n");
    return 0;
}

//open
int do_open(const char * path , struct fuse_file_info * ffi){
    printf("open => path is %s\n", path);
    return 0;
}

//write
int do_write(const char * path, const char * string, size_t size, off_t offset ,struct fuse_file_info * ffi){
    //i think the 1st string is the path 
    printf("writed => path is %s, string is %s, size is %d, offset is %d\n", path,string,size,offset);
    return 0;
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
int do_readdir(const char * path, void * idk, fuse_fill_dir_t idk_either, off_t offset, struct fuse_file_info * ffi){

    printf("readDired -> path is %s\n", path);
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
    return 0;
}


int main(int argc, char* argv[]){
    struct fuse_operations* fops = (struct fuse_operations*)malloc(sizeof(struct fuse_operations));
    fops -> getattr = do_getattr;
    fops -> mkdir = do_mkdir;
    fops -> create = do_create;
    fops -> open = do_open;
    fops -> write = do_write;
    fops -> truncate = do_truncate;
    fops -> opendir = do_opendir;
    fops -> readdir = do_readdir;
    fops -> releasedir = do_releasedir;
    fops -> mkdir = do_mkdir;
    fops -> flush = do_flush;
    fops -> release = do_release;
    return fuse_main( argc, argv, fops, NULL );
}

