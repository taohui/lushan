
CFLAGS := $(CFLAGS) -Wall -O3 -I../../include -I/usr/local/hiredis/include
LIBS := $(LIBS) -L../../lib -lm -L/usr/local/hiredis/lib -lhiredis

### Complie Rules. ###
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC
.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

TARGET = hmodule.so

### Objects. ###
OBJS1 = lproxy.o

all : $(TARGET)

hmodule.so : $(OBJS1)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS1) $(LIBS)

### Clean. ###
clean:
	rm -f $(TARGET) *.o *.a tags
