CC = g++
CFLAGS = -Wall
OBJ = tracker.o
%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

tracker: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lssl -lcrypto -pthread 
 
clean:
	rm -rf *o tracker