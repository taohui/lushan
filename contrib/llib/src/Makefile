CFLAGS := -g -fPIC -Wall -I../include

LIBS := $(LIBS)

### Complie Rules. ###
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC
.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

TARGET = libllib.so

### Objects. ###
OBJS1 = lstring_utils.o fileset.o vlong.o lookup3.o

all : $(TARGET)

libllib.so : $(OBJS1)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS1)

dist : $(TARGET)
	cp libllib.so ../lib/

### Clean. ###
clean:
	rm -f $(TARGET) *.o *.a tags
