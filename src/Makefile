LIBEVENT_HOME =	/usr/local/libevent

CFLAGS := -g -fPIC -Wall -I$(LIBEVENT_HOME)/include -I../include

LIBS := $(LIBS) -L$(LIBEVENT_HOME)/lib -levent -lpthread -ldl -L./ -lstdc++

### Complie Rules. ###
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC
.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

TARGET = lushan liblushan.so

### Objects. ###
OBJS1 = hdict.o hmod.o llog.o url.o lutil.o lnet.o lconf.o
OBJS2 = lushan.o

all : $(TARGET)

lushan : liblushan.so $(OBJS2)
	$(CC) $(CFLAGS) -o $@ $(OBJS2) -llushan $(LIBS)
liblushan.so : $(OBJS1)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS1)

dist : $(TARGET)
	cp lushan ../bin/
	cp liblushan.so ../lib/

### Clean. ###
clean:
	rm -f $(TARGET) *.o *.a tags
