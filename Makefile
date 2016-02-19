A = bigBedSearch

# The following deactivates certain parts of lib/*.c that we don't need.
# They are all marked with #ifndef lines with the TRP_EXCISION symbol.
HG_DEFS = -DTRP_EXCISION

ifeq (${BINDIR},)
	BINDIR = .
endif
HG_INC = -I./include -I./src

########################### adapted from kent/src/inc/common.mk ##############################

CC = gcc
# to build on sundance: CC=gcc -mcpu=v9 -m64
ifeq (${COPT},)
	COPT = -O -g
endif
ifeq (${CFLAGS},)
	CFLAGS =
endif
ifeq (${MACHTYPE},)
	MACHTYPE := $(shell uname -m)
#		$(info MACHTYPE was empty, set to: ${MACHTYPE})
endif
ifneq (,$(findstring -,$(MACHTYPE)))
#		 $(info MACHTYPE has - sign ${MACHTYPE})
	MACHTYPE := $(shell uname -m)
#		 $(info MACHTYPE has - sign set to: ${MACHTYPE})
endif

# to check for Mac OSX Darwin specifics:
UNAME_S := $(shell uname -s)

#global external libraries 
L =


# pthreads is required
ifneq ($(UNAME_S),Darwin)
	L += -pthread
endif

# autodetect if openssl is installed
ifeq (${SSLDIR},)
	SSLDIR = /usr/include/openssl
endif
ifeq (${USE_SSL},)
	ifneq ($(wildcard ${SSLDIR}),)
		USE_SSL = 1
	endif
endif

ifeq (${USE_SSL},1)
	ifneq (${SSL_DIR}, "/usr/include/openssl")
		ifneq ($(UNAME_S),Darwin)
			L += -L${SSL_DIR}/lib
		endif
			HG_INC += -I${SSL_DIR}/include
	endif
	L += -lssl -lcrypto
	HG_DEFS += -DUSE_SSL
endif


SYS = $(shell uname -s)

ifeq (${HG_WARN},)
	ifeq (${SYS},Darwin)
		HG_WARN = -Wall -Wno-unused-variable -Wno-deprecated-declarations
		HG_WARN_UNINIT=
	else
		ifeq (${SYS},SunOS)
			HG_WARN = -Wall -Wformat -Wimplicit -Wreturn-type
			HG_WARN_UNINIT = -Wuninitialized
		else
			ifeq (${FULLWARN},hgwdev)
				HG_WARN = -Wall -Werror -Wformat -Wformat-security -Wimplicit -Wreturn-type -Wempty-body -Wunused-but-set-variable
				HG_WARN_UNINIT = -Wuninitialized
			else
				HG_WARN = -Wall -Wformat -Wimplicit -Wreturn-type
				HG_WARN_UNINIT = -Wuninitialized
			endif
		endif
	endif
	# -Wuninitialized generates a warning without optimization
	ifeq ($(findstring -O,${COPT}),-O)
		HG_WARN += ${HG_WARN_UNINIT}
	endif
endif

CFLAGS += ${HG_WARN}

MKDIR=mkdir -p
ifeq (${STRIP},)
   STRIP=true
endif

# portable naming of compiled executables: add ".exe" if compiled on 
# Windows (with cygwin).
ifeq (${OS}, Windows_NT)
	AOUT=a
	EXE=.exe
else
	AOUT=a.out
	EXE=
endif

%.o: %.c
	${CC} ${COPT} ${CFLAGS} ${HG_DEFS} ${HG_INC} -o $@ -c $<

########################### adapted from kent/src/inc/userApp.mk ##############################

#DEPLIBS = ./lib/${MACHTYPE}/jkweb.a
#LINKLIBS = ${DEPLIBS} -lm
LINKLIBS = 

L += -lm -lz ${SOCKETLIB}

# These are all source files in lib/
_OBJ = asParse.o bbiRead.o cirTree.o errAbort.o hmmstats.o linefile.o \
	options.o rbTree.o verbose.o bPlusTree.o bigBed.o common.o filePath.o htmshell.o \
	localmem.o pipeline.o sqlNum.o zlibFace.o base64.o bits.o dnautil.o hash.o https.o \
	net.o portimpl.o tokenizer.o basicBed.o cheapcgi.o dystring.o hex.o internet.o obscure.o \
	rangeTree.o udc.o sqlList.o binRange.o mime.o osunix.o memalloc.o dlist.o intExp.o kxTok.o \
	wildcmp.o
extraObjects = $(patsubst %,lib/%,$(_OBJ))
extraObjects += src/bigBedPrefixSearch.o

O = ${A}.o
objects = ${O} ${extraObjects}


${BINDIR}/${A}${EXE}: ${DEPLIBS} ${O} ${extraObjects}
	${CC} ${COPT} -o ${BINDIR}/${A}${EXE} ${objects} ${LINKLIBS} ${L}
	${STRIP} ${BINDIR}/${A}${EXE}

${BINDIR}:
	${MKDIR} ${BINDIR}

compile:: ${DEPLIBS} ${O} ${extraObjects}
	${CC} ${COPT} ${CFLAGS} -o ${A}${EXE} ${objects} ${LINKLIBS} ${L}

install:: compile
	rm -f ${BINDIR}/${A}${EXE}
	cp -p ${A}${EXE} ${BINDIR}/${A}${EXE}
	${STRIP} ${A}${EXE} ${BINDIR}/${A}${EXE}
	rm -f ${O} ${A}${EXE}

clean::
	rm -f ${O} ${extraObjects} ${A}${EXE}

