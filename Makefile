TARGET1 = netfileserver
TARGET2 = client
OBJECTS1 = netfileserver.o
OBJECTS2 = libnetfiles.o client.o
FLAGS = -Wall -g 

all: $(TARGET2) $(TARGET2)

$(TARGET1): $(OBJECTS1)
	gcc $(FLAGS) -lpthread -o $@ $^

$(TARGET2): $(OBJECTS2)
	gcc $(FLAGS) -o $@ $^

clean: 
	rm -f $(TARGET1) $(TARGET2) $(OBJECTS1) $(OBJECTS2)

%.o: %.c
	gcc $(FLAGS) -c $<

libnetfiles.o: libnetfiles.h
client.o: libnetfiles.h

