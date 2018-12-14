DIR="/tmp/fuse"
echo "" > res2.txt

#echo "file1" > $DIR/file1.txt
#echo "file2" > $DIR/file2.txt
#echo "file3" > $DIR/file3.txt

mkdir $DIR/dir1
mkdir $DIR/dir2

#mkdir $DIR/dir3
#echo -e "file4" > $DIR/dir3/file4.txt
#echo -e "file5" > $DIR/dir4/file5.txt

echo -e "result of regular ls on $DIR \n" >> res2.txt
ls $DIR/ >> res2.txt
echo -e "\n\n\n" >> res2.txt

echo -e "result of ls -a on $DIR \n" >> res2.txt
ls -a $DIR/ >> res2.txt
echo -e "\n\n\n" >> res2.txt

echo -e "result of ls -la on $DIR \n" >> res2.txt
ls -la $DIR/ >> res2.txt
echo -e "\n\n\n" >> res2.txt

#echo -e "result of ls -l on $DIR/dir3\n" > res2.txt
#ls -l $DIR/dir3 > res2.txt
#echo -e "\n\n\n" > res2.txt

