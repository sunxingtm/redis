uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
osname := $(shell uname -rs | cut -f 1-2 -d "." | cut -f 1 -d "-")

ifeq ($(osname),MINGW32_NT)
  CFLAGS?= -std=gnu99 -pedantic $(OPTIMIZATION) -Wall -W -D_ISOC99_SOURCE -D__USE_MINGW_ANSI_STDIO=1 -Wwrite-strings $(ARCH) $(PROF)
  CCLINK?= -mthreads
  CC=gcc
endif  

linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.o example.o
	$(CC) $(ARCH) -Wall -W -Os -g -o linenoise_example linenoise.o example.o

.c.o:
	$(CC) $(ARCH) -c -Wall -W -Os -g $<

clean:
	rm -f linenoise_example *.o *.exe
