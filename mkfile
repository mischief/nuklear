</$objtype/mkfile

CFLAGS=-FTV -p
LIB=libnuklear.$O.a
OFILES=nuklear.$O
HFILES=nuklear.h

</sys/src/cmd/mksyslib

nuklear.h:D:	nuklear_defines.h upstream/nuklear.h nuklear_plan9.h
	@{
		cat nuklear_defines.h
		grep -v '^#include' upstream/nuklear.h
		cat nuklear_plan9.h
	} > $target

nuke:V:
	rm -f *.[$OS] [$OS].* $LIB nuklear.h

install:V:	$LIB $HFILES
	cp $LIB /$objtype/lib/libnuklear.a
	cp nuklear.h /sys/include/nuklear.h

uninstall:V:
	rm -f /$objtype/lib/libnuklear.a
	rm -f /sys/include/nuklear.h

sync:V:
	rm -f upstream/nuklear.h
	hget -o upstream/nuklear.h https://raw.githubusercontent.com/vurtun/nuklear/master/nuklear.h
