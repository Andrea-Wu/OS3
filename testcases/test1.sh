echo "ehehehe" > /tmp/fuse/cpMe.txt
cp /tmp/fuse/cpMe.txt /tmp/fuse/cpMe2.txt
cat /tmp/fuse/cpMe2.txt
diff /tmp/fuse/cpMe.txt /tmp/fuse/cpMe2.txt > res1.txt
