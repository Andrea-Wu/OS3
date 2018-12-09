all:
	gcc -pthread netfileserver.c -o netfileserver
	gcc -pthread testclient.c libnetfiles.h libnetfiles.c -o testclient
	gcc -pthread TransactionRW.c libnetfiles.h libnetfiles.c -o TransactionRW
	gcc -pthread ExclusiveR.c libnetfiles.h libnetfiles.c -o ExclusiveR
	gcc -pthread ExclusiveRW.c libnetfiles.h libnetfiles.c -o ExclusiveRW
	gcc -pthread UnrestrictedR.c libnetfiles.h libnetfiles.c -o UnrestrictedR
	gcc -pthread UnrestrictedRW.c libnetfiles.h libnetfiles.c -o UnrestrictedRW

clean:
	rm -f netfileserver
	rm -f testclient
	rm -f TransactionRW
	rm -f ExclusiveR
	rm -f ExclusiveRW
	rm -f UnrestrictedR
	rm -f UnrestrictedRW
