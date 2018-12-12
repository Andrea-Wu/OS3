all:
	gcc -pthread netfileserver.c -o netfileserver

clean:
	rm -f netfileserver
