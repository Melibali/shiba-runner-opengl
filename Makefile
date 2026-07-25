#  Makefile 

CC      = gcc
ECHO    = echo
RM      = rm -f
TAR     = tar
ZIP     = zip
MKDIR   = mkdir
CHMOD   = chmod
CP      = rsync -R

CFLAGS   = -Wall -O3
CPPFLAGS = -I.
LDFLAGS  = -lm

PACKNAME = L2-Y
PROGNAME = demo
VERSION  = 1.0
distdir  = $(PACKNAME)_$(PROGNAME)-$(VERSION)

HEADERS = animations.h audioHelper.h assimp.h 
SOURCES = animations.c audioHelper.c window.c assimp.c \
          shiba.c skydome2.c logo.c shiba_spiral.c

MSVCSRC  = $(patsubst %,<ClCompile Include=\"%\\\" \/>,$(SOURCES))
OBJ      = $(SOURCES:.c=.o)

DOXYFILE   = documentation/Doxyfile
VSCFILES   = $(PROGNAME).vcxproj $(PROGNAME).sln
EXTRAFILES = COPYING DejaVuSans-Bold.ttf $(wildcard shaders/*.?s images/*) \             $(shell find models -type f) $(VSCFILES) takeonme.mod
DISTFILES  = $(SOURCES) Makefile $(HEADERS) $(DOXYFILE) $(EXTRAFILES)

ifneq (,$(shell ls -d /usr/local/include 2>/dev/null | tail -n 1))
    CPPFLAGS += -I/usr/local/include
endif
ifneq (,$(shell ls -d /opt/local/include 2>/dev/null | tail -n 1))
    CPPFLAGS += -I/opt/local/include
endif
ifneq (,$(shell ls -d $(HOME)/local/include 2>/dev/null | tail -n 1))
    CPPFLAGS += -I$(HOME)/local/include
endif
ifneq (,$(shell ls -d /usr/local/lib 2>/dev/null | tail -n 1))
    LDFLAGS += -L/usr/local/lib
endif
ifneq (,$(shell ls -d /opt/local/lib 2>/dev/null | tail -n 1))
    LDFLAGS += -L/opt/local/lib
endif
ifneq (,$(shell ls -d $(HOME)/local/lib 2>/dev/null | tail -n 1))
    LDFLAGS += -L$(HOME)/local/lib
endif

ifeq ($(shell uname),Darwin)
    MACOSX_DEPLOYMENT_TARGET = 11.0
    CFLAGS  += -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
    LDFLAGS += -framework OpenGL \
               -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
else
    LDFLAGS += -lGL
endif

CPPFLAGS += $(shell sdl2-config --cflags)
LDFLAGS  += -lGL4Dummies $(shell sdl2-config --libs) \
            -lSDL2_mixer -lSDL2_image -lSDL2_ttf -lassimp

all: $(PROGNAME)

$(PROGNAME): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(PROGNAME)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

dist: distdir
	$(CHMOD) -R a+r $(distdir)
	$(TAR) zcvf $(distdir).tgz $(distdir)
	$(RM) -r $(distdir)

zip: distdir
	$(CHMOD) -R a+r $(distdir)
	$(ZIP) -r $(distdir).zip $(distdir)
	$(RM) -r $(distdir)

distdir: $(DISTFILES)
	$(RM) -r $(distdir)
	$(MKDIR) $(distdir)
	$(CHMOD) 777 $(distdir)
	$(CP) $(DISTFILES) $(distdir)

clean:
	@$(RM) -f *.o $(PROGNAME) *~ $(distdir).tgz $(distdir).zip \
	      gmon.out core.* documentation/*~ shaders/*~ documentation/html