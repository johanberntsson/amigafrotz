# Assuming vc amiga cross compiler installed and configured. See:
# https://blitterstudio.com/setting-up-an-amiga-cross-compiler/
#
# Note that it is necessary to install the kick13 target as described in
# https://linuxjedi.co.uk/2021/02/27/using-vbcc-as-an-amiga-cross-compiler-in-linux/
#
export VBCC := /opt/vbcc
export PATH := ${VBCC}/bin:$(PATH)

# Amitools for creating Amiga floppies
AMITOOLS = /home/johan/amiga/amitools
export PATH := ${AMITOOLS}/bin:$(PATH)

CC=vc +kick13
CFLAGS=-c99 -Ireqtools -DNDEBUG -lamiga -lauto

TARGETS := frotz

all: $(TARGETS)

frotz:
	${CC} ${CFLAGS} -o frotz amiga.c

floppy: frotz
	echo "frotz" > startup-sequence
	xdftool frotz.adf format frotz + boot install boot1x + makedir c + write frotz + makedir s + write startup-sequence s/startup-sequence

run: floppy
	fs-uae frotz.adf

clean:
	rm -f $(TARGETS) frotz.adf
