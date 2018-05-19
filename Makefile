#ARCH=arm
#CC="/home/albert/buildroot/buildroot-2017.11/output/host/bin/arm-buildroot-linux-uclibcgnueabihf-gcc"

# 0: use ALSA,  1: use tinyalsa 
SUPPORT_TINYALSA=0

CFLAGS=-Wall -I/usr/include/alsa/ -I.
ifeq ($(SUPPORT_TINYALSA), 1)
CFLAGS+=-I../tinyalsa/include/ -DSUPPORT_TINYALSA=1
endif

LDFLAGS=-lasound -ldl -lm

SRC=$(wildcard *.c)
OBJECT= ${SRC:.c=.o}

all: aplayer arecorder
%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

aplayer: my_alsa_player.o my_alsa_common.o
	$(CC) -o $@ my_alsa_player.o my_alsa_common.o $(LDFLAGS) 

arecorder: my_alsa_recorder.o my_alsa_common.o
	$(CC) -o $@ my_alsa_recorder.o my_alsa_common.o $(LDFLAGS) 

clean:
	rm -f *.o arecorder aplayer