CFLAGS = -Wall -g -Werror -Wno-error=unused-variable
COMMON_DIR = include

all: server subscriber

# Compile server.c and link with common.o
server: server.c $(COMMON_DIR)/common.o
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -o $@ $^

# Compile subscriber.c and link with common.o
subscriber: subscriber.c $(COMMON_DIR)/common.o
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -o $@ $^

# Compile common.c into common.o
$(COMMON_DIR)/common.o: $(COMMON_DIR)/common.c $(COMMON_DIR)/common.h
	$(CC) $(CFLAGS) -I$(COMMON_DIR) -c $< -o $@

# Run the server
run_server: server
	./server

# Run the subscriber
run_subscriber: subscriber
	./subscriber

# Clean up
clean:
	rm -rf server subscriber $(COMMON_DIR)/*.o *.dSYM
