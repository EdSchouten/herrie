# 
# Copyright (c) 2006-2007 Ed Schouten <ed@fxq.nl>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# 

APP_NAME=	herrie
VERSION=	1.2

PREFIX?=	/usr/local
OS:=		$(shell uname)
MANDIR=		$(PREFIX)/share/man
TRANSDIR=	$(PREFIX)/share/locale

AUDIO_OUTPUT=	ao

VPATH=		lang:src

# Operating system specific options
ifeq ($(OS),Darwin)
CFLAGS+=	-DBUILD_GUI_SIGWINCH_WRAPPER
LDFLAGS+=	-lresolv
PREFIX=		/opt/local
endif
ifeq ($(OS),FreeBSD)
MANDIR=		$(PREFIX)/man
AUDIO_OUTPUT=	oss
endif
ifeq ($(OS),Linux)
CFLAGS+=	-D_GNU_SOURCE
PREFIX=		/usr
AUDIO_OUTPUT=	oss
endif
ifeq ($(OS),NetBSD)
PREFIX=		/usr/pkg
AUDIO_OUTPUT=	oss
endif
ifeq ($(OS),OpenBSD)
MANDIR=		$(PREFIX)/man
AUDIO_OUTPUT=	oss
endif
ifeq ($(OS),SunOS)
CFLAGS+=	-I$(PREFIX)/include/ncurses
PREFIX=		/opt/sfw
NO_HTTP=	yes
NO_TRANS=	yes
endif
ifneq (,$(findstring CYGWIN,$(OS)))
BINARY_EXT=	.exe
PREFIX=		/usr
NO_HTTP=	yes
endif 

# Audio output specific options
ifeq ($(AUDIO_OUTPUT),ao)
# libao outputs debugging info to stderr on some systems
CFLAGS+=	-DCLOSE_STDERR
LDFLAGS+=	-lao
endif
ifeq ($(AUDIO_OUTPUT),oss)
ifeq ($(OS),NetBSD)
LDFLAGS+=	-lossaudio
endif
ifeq ($(OS),OpenBSD)
CFLAGS+=	-DBUILD_GUI_SIGWINCH_WRAPPER
LDFLAGS+=	-lossaudio
endif
endif
ifeq ($(AUDIO_OUTPUT),sdl)
CFLAGS+=	$(shell sdl-config --cflags)
LDFLAGS+=	$(shell sdl-config --libs)
endif

# Tunables
ifndef NO_HTTP
OBJS+=		vfs_http.o
CFLAGS+=	-DBUILD_HTTP
CURL=		yes
endif
ifndef NO_MP3
OBJS+=		audio_format_mp3.o
CFLAGS+=	-DBUILD_MP3
LDFLAGS+=	-lmad -lid3tag
endif
ifndef NO_VORBIS
OBJS+=		audio_format_vorbis.o
CFLAGS+=	-DBUILD_VORBIS
LDFLAGS+=	-lvorbisfile -lvorbis -logg
endif
ifndef NO_SNDFILE
OBJS+=		audio_format_sndfile.o
CFLAGS+=	-DBUILD_SNDFILE
LDFLAGS+=	-lsndfile
endif
ifndef NO_SCROBBLER
OBJS+=		scrobbler.o scrobbler_hash.o scrobbler_send.o
CFLAGS+=	-DBUILD_SCROBBLER
LDFLAGS+=	-lssl
CURL=		yes
endif
ifndef NO_TRANS
CFLAGS+=	-DBUILD_TRANS -DTRANSDIR=\"$(TRANSDIR)\"
LOCALES=	nl
endif
ifdef CURL
CFLAGS+=	$(shell curl-config --cflags) -DBUILD_SCROBBLER
LDFLAGS+=	$(shell curl-config --libs) -lssl
endif
ifdef XCURSES
CFLAGS+=	-DBUILD_XCURSES
LDFLAGS+=	-lXCurses
else
ifeq ($(OS),SunOS)
LDFLAGS+=	-lncurses
else
LDFLAGS+=	-lcurses
endif
endif

OBJS+=		audio_file.o audio_output_$(AUDIO_OUTPUT).o config.o \
		gui_browser.o gui_draw.o gui_input.o gui_msgbar.o \
		gui_playq.o gui_vfslist.o main.o playq.o vfs.o \
		vfs_playlist.o vfs_regular.o
# libao dies with -Wstrict-prototypes - fixed in trunk
CFLAGS+=	$(shell pkg-config --cflags glib-2.0 gthread-2.0) \
		-I$(PREFIX)/include -DAUDIO_OUTPUT=\"$(AUDIO_OUTPUT)\" \
		-DAPP_NAME=\"$(APP_NAME)\" -DAPP_VERSION=\"$(VERSION)\" \
		-Wall -Wsign-compare -Werror -Wmissing-prototypes
LDFLAGS+=	$(shell pkg-config --libs glib-2.0 gthread-2.0) \
		-L$(PREFIX)/lib

.PHONY: all install uninstall distclean clean depends

all: $(APP_NAME) $(APP_NAME).1.gz $(LOCALES:%=%.mo)

install: all
	install -s $(APP_NAME) $(PREFIX)/bin/$(APP_NAME)$(BINARY_EXT)
	install -m 444 $(APP_NAME).1.gz $(MANDIR)/man1/
	$(foreach lang,$(LOCALES), \
		mkdir -p $(TRANSDIR)/$(lang)/LC_MESSAGES)
	$(foreach lang,$(LOCALES), \
		install -m 444 $(lang).mo \
		    $(TRANSDIR)/$(lang)/LC_MESSAGES/$(APP_NAME).mo)

clean:
	rm -f *.core *.mo *.o .gdb_history

distclean: clean
	rm -f src/stdinc.h.gch $(APP_NAME) $(APP_NAME).1.gz

%.h.gch: %.h
	-$(CC) $(CFLAGS) $<
%.o: %.c
	$(CC) -c $(CFLAGS) -include src/stdinc.h $<

%.mo: %.po
	msgfmt -o $@ $<

%.gz: %
	gzip -9 < $< > $@

$(APP_NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

include depends.mak
