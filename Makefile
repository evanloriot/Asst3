TARGET = client
OBJECTS = netfileserver.o libnetfiles.o client.o
FLAGS = -Wall -g 

$(TARGET): $(OBJECTS)
	gcc $(FLAGS) -lpthread -o $@ $^

clean: 
	rm -f $(TARGET) $(OBJECTS)

%.o: %.c
	gcc $(FLAGS) -c $<

libnetfiles.o: libnetfiles.h
client.o: libnetfiles.h

