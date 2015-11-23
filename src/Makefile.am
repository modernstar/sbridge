#  SBridge
#  Sangoma WAN - TCP/IP Bridge
# 
#  Author(s):   Moises Silva <moises.silva@gmail.com>
# 
#  Copyright:   (c) 2009 Regulus Labs Ltd
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version
#  2 of the License, or (at your option) any later version.
#  

AM_CFLAGS = -std=c99 -pedantic -Wall -Werror -Wwrite-strings -Wunused-variable -Wstrict-prototypes -Wmissing-prototypes 
AM_CFLAGS += -I/usr/include -I/usr/src/libpri -I$(WANINC) -I. -I../include/ -I../src/v32/

# required by libsangoma headers
AM_CFLAGS += -D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O0 -DAFT_A104

if WANT_DEBUG
AM_CFLAGS += -DSBRIDGE_DEBUG
endif


# sbridge binary
bin_PROGRAMS = sbridge

# sources
sbridge_SOURCES = sbridge_main.c sbridge_config.c sbridge_log.c sbridge_os.c sbridge_v110.c sbridge_v32.c sbridge_media.c sangoma_pri.c \
	v32/agc.c      \
	v32/anstxsm.c \
	v32/calrxsm.c \
	v32/dcmf.c    \
	v32/echocan.c \
	v32/generat.c \
	v32/mapper.c  \
	v32/rate.c    \
	v32/scram.c   \
	v32/slicer.c  \
	v32/transmt.c \
	v32/ansrxsm.c \
	v32/BackTrk.c \
	v32/caltxsm.c \
	v32/detect.c  \
	v32/equ.c     \
	v32/Goertzl.c \
	v32/packing.c \
	v32/ratedet.c \
	v32/shape.c   \
	v32/tables.c  \
	v32/viterbi.c \
	v32/anstone.c \
	v32/brncmet.c \
	v32/conv.c    \
	v32/diff.c    \
	v32/filter.c  \
	v32/initmdm.c \
	v32/PathMet.c \
	v32/receive.c \
	v32/SincTbl.c \
	v32/timing.c  \
	v32/vtrTbl.c  \
	v32/v32_int.c  \
	v14/v14.c

# sbridge flags
sbridge_LDADD = -lpthread -lsangoma -lpri -lsqlite3
sbridge_CFLAGS = $(AM_CFLAGS)

