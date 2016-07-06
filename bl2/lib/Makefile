CC      = arm-linux-gnueabi-gcc
LD      = arm-linux-gnueabi-ld
AR      = arm-linux-gnueabi-ar
OBJCOPY = arm-linux-gnueabi-objcopy
OBJDUMP = arm-linux-gnueabi-objdump
INCLUDEDIR      := $(shell pwd)/../include
CFLAGS          := -Wall -O2 -fno-builtin
CPPFLAGS        := -nostdinc -I$(INCLUDEDIR)

objs := div64.o lib1funcs.o ctype.o muldi3.o printf.o string.o vsprintf.o stdio.o

libc.a: $(objs)
	${AR} -r -o $@ $^
	
%.o:%.c
	${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o:%.S
	${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f libc.a *.o		
	
