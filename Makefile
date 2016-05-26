CC = g++
CFLAGS = -Wall -mwindows -static-libgcc -static-libstdc++
OBJECTS =  main.o 
LDFLAGS= -lws2_32


client.exe: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o client.exe $(LDFLAGS)

all:client.exe

.PHONY: clean
clean:
	rm -f *~ *.o client.exe
