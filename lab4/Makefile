all:
	gcc -Wall -g3 -fsanitize=address -pthread server.c threadpool.c -o server
	gcc -Wall -g3 -fsanitize=address -pthread client.c -o client

work:
	gcc -Wall -g3 -fsanitize=address -pthread work.c threadpool.c -o work
	
clean:
	$(RM) server client
