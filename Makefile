httpServer : httpServer.o
	gcc -o ./bin/httpServer httpServer.o
httpServer.o : 
	gcc -c ./src/httpServer.c --std=gnu99
clean : 
	rm ./bin/httpServer ./httpServer.o
