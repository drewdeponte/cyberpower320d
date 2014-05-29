# cyberpower320d: a UPS monitor for the CyberPower 320LS UPS
# Author: Andrew De Ponte (CyPhAcToR)
#
# =========================================================
# Local Configuration
# =========================================================
# The directory where cyberpower320d will be installed.
INSTALLDIR = /usr/local/sbin/
# The directory where power status messages will be stored.
POWSTATDIR = /usr/local/share/cyberpower320d/

# =========================================================
# Configurable but highly discuraged
# =========================================================
SRC = src/cyberpower320d.c
OBJ = cyberpower320d.o
BIN = cyberpower320d
POKFILE = pstat_msgs/pok.txt
PFAILFILE = pstat_msgs/pfail.txt
PLOWFILE = pstat_msgs/plow.txt

# =========================================================
# No user config below
# =========================================================
cyberpower320d : ${OBJ}
	gcc ${OBJ} -o ${BIN}

cyberpower320d.o : ${SRC}
	gcc -c ${SRC}

clean :
	rm ${OBJ} ${BIN}

install : ${BIN}
	cp ${BIN} ${INSTALLDIR}
	mkdir ${POWSTATDIR}
	cp ${POKFILE} ${PFAILFILE} ${PLOWFILE} ${POWSTATDIR}

# END OF FILE
