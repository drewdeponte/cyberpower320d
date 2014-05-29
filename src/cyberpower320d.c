/*
 * Copyright (c) 2001, 2002, 2003 Andrew De Ponte.
 * 
 * This file is part of cyberpower320d.
 * 
 * cyberpower320d is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * cyberpower320d is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cyberpower320d; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/******************************************************************************
 * FileName:    cyberpower320d.c
 * Description: A UPS monitor for the CyberPower 320SL Intelligent
 *              Back-Up Power, this is written sepecificly for
 *              the 320SL and has not been tested on any other
 *              UPSs. However many people have told me that it works
 *              on the following Cyber Power Systems as well, Power99
 *              and Power2000
 * Author:      Andrew De Ponte (cyphactor@socal.rr.com)
 ******************************************************************************/

#include <stdio.h>      /* sprintf() */
#include <sys/types.h>  /* open() */
#include <sys/stat.h>   /* open() */
#include <fcntl.h>      /* open() */
#include <stdlib.h>     /* exit(), system() */
#include <unistd.h>     /* sleep(), daemon() */
#include <sys/ioctl.h>  /* ioctl() */
#include <string.h>     /* strcpy() */

#define TESTINT 5       /* Test UPS polling interval, in seconds */
#define POLLCNT 5       /* State change debouncing counter. */

/* Defines for UPS status. */
#define POK 0
#define PFAIL 1
#define PLOW 2

uint failmask = 0, failtest = 0;
uint battmask = 0, batttest = 0;
uint setmask = ~0, setbits = 0;
uint killmask = ~0, killbits = 0;

int init_line(char *device);
int read_line(int line);

int main(int argc, char *argv[]) {
    /* argv[0] = program name
     * argv[1] = serial port
     * argv[2] = shut down script */

    int line;               /* the file descript for the serial line */
    unsigned int bits;      /* for serial communication */
    int lastStatus = POK;   /* UPS status */
    int newStatus;
    int bounceCnt;          /* Debouncing counter */
    char cmdBuff[256];      /* Command Buffer */
    char pokfile[256];      /* Variable holding the full path for pok */
    char plowfile[256];     /* Variable holding the full path for plow */
    char pfailfile[256];    /* Variable holding the full path for pfail */
    char wallCmd[256];      /* Variable holding the full path to wall */

    if (argc != 3) {
        printf("usage: %s <serial port> <shutdown script>\n", argv[0]);
        exit(0);
    }

    /* daemon(0, 0); */

    strcpy(wallCmd, "/usr/bin/wall");
    strcpy(pokfile, "/usr/local/share/cyberpower320d/pok.txt");
    strcpy(pfailfile, "/usr/local/share/cyberpower320d/pfail.txt");
    strcpy(plowfile, "/usr/local/share/cyberpower320d/plow.txt");

/* init rts 1 */
    setmask &= ~TIOCM_RTS;
    setbits |= TIOCM_RTS;
      
/* fail cts 0 */
    failtest = TIOCM_CTS;
    failmask = TIOCM_CTS;

/* low dcd 0 */
    batttest = TIOCM_CD;
    battmask = TIOCM_CD;

/* init dtr 0 */
    setmask &= ~TIOCM_DTR;
    setbits |= 0;

/* kill dtr 1 */
    killmask &= ~TIOCM_DTR;
    killbits |= TIOCM_DTR;

    line = init_line(argv[1]);

    daemon(0, 0);

    if (setmask != ~0) {
        ioctl (line, TIOCMGET, &bits);
        bits = (bits & setmask) | setbits;
        ioctl (line, TIOCMSET, &bits);
    }

    bounceCnt = POLLCNT;
    while (1) {
        newStatus = read_line(line);
        bounceCnt = POLLCNT;
        while ((lastStatus != newStatus) && bounceCnt-- > 0) {
            newStatus = read_line (line);

            /* Sleep for reduced polling interval. */
            sleep (TESTINT);
        }
        if (lastStatus != newStatus) {
            /* Accept state change. */
            if (newStatus == POK) {
                sprintf(cmdBuff, "%s %s", wallCmd, pokfile);
                system(cmdBuff);
            } else if (newStatus == PFAIL) {
                sprintf(cmdBuff, "%s %s", wallCmd, pfailfile);
                system(cmdBuff);
            } else if (newStatus == PLOW) {
                sprintf(cmdBuff, "%s %s", wallCmd, plowfile);
                system(cmdBuff);

                /* send the kill signal to the UPS */
                ioctl (line, TIOCMGET, &bits);
                bits = (bits & killmask) | (~killbits & ~killmask);
                ioctl (line, TIOCMSET, &bits);
                /* sleep (TESTINT); */

                /* set the killbits */
                ioctl (line, TIOCMGET, &bits);
                bits = (bits & killmask) | killbits;
                ioctl (line, TIOCMSET, &bits);

                /* execute shut down script */
                system(argv[2]);
            }
            lastStatus = newStatus;
        }
        /* Sleep to reduce the polling */
        sleep (TESTINT);
    }
} /* end of main() */

/* init_line()
 * opens the serial port and checks for error */
int init_line(char *device) {
    int line;
    unsigned int bits;

    /* Open serial line. If unsuccessful exit. */
    if ((line = open (device, O_RDWR | O_NDELAY)) == -1) {
        printf("open: ERROR - failed to open the serial port\n");
        exit(0);
    }

    return (line);
} /* end of init_line() */

/* read_line()
 * read the bits from the line, interpret the bits and return the status */
int read_line(int line) {
    unsigned int bits;
    int status = POK;

    /* Obtain bits from serial line */
    ioctl (line, TIOCMGET, &bits);

    /* Interpret bits according to configuration. Note that the low
     * batter signal is orthogonal to power failure. If the power is
     * off, we want to signal PLOW, but if the power is on we want to
     * signal POK. */
    if ((bits & failmask) ^ failtest) {
        if ((bits & battmask) ^ batttest)
            status = PLOW;
        else
            status = PFAIL;
    }

    return (status);
}/* end of read_line() */
