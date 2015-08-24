LIBEVENT_HOME =	/usr/local/libevent

CFLAGS := -g -Wall -I$(LIBEVENT_HOME)/include

LIBS := $(LIBS) -L$(LIBEVENT_HOME)/lib -levent -lpthread

### Complie Rules. ###
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC
.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

TARGET = lushan

### Objects. ###
OBJS1 = hdict.o lushan.o

all : $(TARGET)

lushan : $(OBJS1)
	$(CC) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)


### Clean. ###
clean:
	rm -f $(TARGET) *.o *.a tags
