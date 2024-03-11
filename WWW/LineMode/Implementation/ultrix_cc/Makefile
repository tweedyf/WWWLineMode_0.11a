#  Make WWW under unix for VAX/ultrix using cc
#
# This directory:
HERE = /userd/tbl/hypertext/WWW/LineMode/Implementation/ultrix_cc

# The directory with the deafult home page in:
DEFAULTS = /userd/tbl/hypertext/WWW/LineMode/Defaults

# Directory for installed binary:
BINDIR = /usr/local/unix


CMN = ../../../Implementation/
LM = ../
COMMON = HTParse.o HTAccess.o HTTP.o
OBJS = www.o $(COMMON) HTBufferFile.o

CFLAGS = -I$(CMN) -I$(LM)
#CFLAGS = -I$(CMN) -I$(LM) -g  -DDEBUG

CC = cc

include $(LM)CommonMakefile
