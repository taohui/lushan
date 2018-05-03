
CFLAGS := $(CFLAGS) -Wall -O3 -I../../include -I/usr/local/libmemcached/include
LIBS := $(LIBS) -L../../lib -llushan -lm -L/usr/local/libmemcached/lib -lmemcached

### Complie Rules. ###
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC
.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

TARGET = hmodule.so lecho_cli

### Objects. ###
OBJS1 = lecho.o
OBJS2 = lecho_cli.o

all : $(TARGET)

hmodule.so : $(OBJS1)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS1) $(LIBS)
lecho_cli : $(OBJS2)
	$(CC) $(CFLAGS) -o $@ $(OBJS2) $(LIBS)

### Clean. ###
clean:
	rm -f $(TARGET) *.o *.a tags
