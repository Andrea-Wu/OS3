gcc clientSNFS.c -o client `pkg-config fuse --cflags --libs` 
rm -rf /tmp/fuse/*
./client -f  /tmp/fuse > log.txt     
