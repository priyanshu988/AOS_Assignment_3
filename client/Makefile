CC = g++
CFLAGS = -Wall
OBJ = client.o
%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lssl -lcrypto -pthread 
 
clean:
	rm -rf *o client