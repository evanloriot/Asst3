TARGET1 = netfileserver
TARGET2 = client
TARGET3 = client2
OBJECTS1 = netfileserver.o
OBJECTS2 = libnetfiles.o client.o
OBJECTS3 = libnetfiles.o client2.o
FLAGS = -Wall -g -lm 

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(OBJECTS1)
	gcc $(FLAGS) -lpthread -o $@ $^

$(TARGET2): $(OBJECTS2)
	gcc $(FLAGS) -o $@ $^

$(TARGET3): $(OBJECTS3)
	gcc $(FLAGS) -o $@ $^

clean: 
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(OBJECTS1) $(OBJECTS2) $(OBJECTS3)

%.o: %.c
	gcc $(FLAGS) -c $<

libnetfiles.o: libnetfiles.h
client.o: libnetfiles.h
client2.o: libnetfiles.h

