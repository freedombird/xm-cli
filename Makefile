# compile for  xiami.com client
# Copyright 2011 SNDA
#


#PLATFORM=arm

ifeq ($(PLATFORM), arm)
CC = /opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-g++
LD = /opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-ld
CXX = $(CC)
else
CC = gcc
LD = ld
CXX = g++
endif

PROG = xiami_cli
LIB_JSON_DIR= lib_json_0.5

SRCS = ${LIB_JSON_DIR}/json_value.cpp ${LIB_JSON_DIR}/json_reader.cpp ${LIB_JSON_DIR}/json_writer.cpp

SRCS+= data_manager.cpp xiami_cli.cpp

OBJS = ${LIB_JSON_DIR}/json_value.o ${LIB_JSON_DIR}/json_reader.o ${LIB_JSON_DIR}/json_writer.o
OBJS += data_manager.o xiami_cli.o

ifneq ($(PLATFORM),arm)
LOCALLDFLAGS += -lstdc++  -lrt -L./curl-7.21.7/host -lcurl -lpthread 
else
LOCALLDFLAGS += -L./curl-7.21.7/arm -lcurl -lz -lpthread
endif

CFLAGS += -Wall -g -I./lib_json_0.5 -I./curl-7.21.7/include/curl
CXXFLAGS += -Wall -g #-I./lib_json_0.5 -I./curl-7.21.7/include/curl

#CPPFLAGS += $(INCLUDE)

all: $(PROG)

$(PROG): $(OBJS)
	$(CXX)  $(CFLAGS)  -o $@ $^ $(LOCALLDFLAGS)

install:
	install  $(PROG) /bin/$(PROG) 
clean:
	rm -f $(PROG) $(OBJS) 

.PHONY: all install clean

