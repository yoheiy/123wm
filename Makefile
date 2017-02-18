LDFLAGS = -lX11
CFLAGS  = -Wall
o = 123wm

all: $o

clean:
	$(RM) $o
