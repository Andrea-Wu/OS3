gcc clientSNFS.c -o myClient `pkg-config fuse --cflags --libs` 
rm -rf /tmp/fuse/*
./myClient -f -port=1010 -address=0.0.0 -mountpath=/tmp/fuse /tmp/fuse > log.txt     
