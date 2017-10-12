#CC=mipsel-openwrt-linux-gcc
CC=gcc
TARGET = server
OBJECT = test.o open62541.o
CFLAGS = -g -std=c99 -lrt -rdynamic 
$(TARGET):$(OBJECT)
	$(CC) -o $@ $^ $(CFLAGS) -lpthread 
$(OBJECT):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
	 
clean:
	rm -f $(TARGET) $(OBJECT)
	
