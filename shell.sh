gcc clientSNFS.c -o myClient `pkg-config fuse --cflags --libs` 
rm -rf /tmp/fuse/*
./myClient -f  /tmp/fuse > log.txt     
