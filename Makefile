TARGET = netfileserver
OBJECTS = netfileserver.o 
FLAGS = -Wall -g 

$(TARGET): $(OBJECTS)
	gcc $(FLAGS) -lpthread -o $@ $^

clean: 
	rm -f $(TARGET) $(OBJECTS)

%.o: %.c
	gcc $(FLAGS) -c $<

libnetfiles.o: libnetfiles.h


