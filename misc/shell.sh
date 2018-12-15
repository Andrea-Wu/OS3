gcc clientSNFS.c -o myClient `pkg-config fuse --cflags --libs` 
rm -rf /tmp/fuse/*
./myClient --port 66666 --address facade.cs.rutgers.edu -f /tmp/fuse > log.txt     
