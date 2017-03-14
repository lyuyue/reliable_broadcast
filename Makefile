# complier
CC = gcc

# compiler flags
CFLAGS = -g -Wall -pthread

# building target
TARGET = proj2

# source file
SOURCE = main

# clean
RM = rm

all: $(TARGET)
	chmod 777 $(TARGET)

$(TARGET): $(SOURCE).c
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE).c

clean:
	$(RM) $(TARGET)
	$(RM) -rf $(TARGET).dSYM