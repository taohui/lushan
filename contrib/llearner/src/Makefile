TBB_HOME =	/usr/local/tbb
LIBMEMCACHED_HOME =	/usr/local/libmemcached

#CFLAGS := $(CFLAGS) -Wall -O3 -I$(TBB_HOME)/include -I$(LIBMEMCACHED_HOME)/include -I../../../include -I../../llib/include -DUSE_TBB
CFLAGS := $(CFLAGS) -Wall -O3 -I$(TBB_HOME)/include -I$(LIBMEMCACHED_HOME)/include -I../../../include -I../../llib/include
LIBS1 := $(LIBS) -L$(TBB_HOME)/lib -ltbb -L../../../lib -llushan -lm
LIBS2 := $(LIBS) -L$(LIBMEMCACHED_HOME)/lib -lmemcached -L../../../lib -llushan -L../../llib/lib -lllib -lm
LIBS3 := $(LIBS) -lm

### Complie Rules. ###
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC
.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

TARGET = hmodule.so lworker auc_logloss

### Objects. ###
OBJS1 = ps.o random.o
OBJS2 = lworker.o lworker_net.o lworker_evaluate.o samples.o utils.o
OBJS3 = auc_logloss.o utils.o

all : $(TARGET)

hmodule.so : $(OBJS1)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS1) $(LIBS1)

lworker : $(OBJS2)
	$(CXX) $(CFLAGS) -o $@ $(OBJS2) $(LIBS2)

auc_logloss : $(OBJS3)
	$(CXX) $(CFLAGS) -o $@ $(OBJS3)

### Clean. ###
clean:
	rm -f $(TARGET) *.o *.a tags
