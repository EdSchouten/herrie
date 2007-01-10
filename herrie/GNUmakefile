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
NO_TRANS=	yes
endif
ifneq (,$(findstring CYGWIN,$(OS)))
BINARY_EXT=	.exe
PREFIX=		/usr
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

# Tunables
ifndef NO_MP3
HERRIE_OBJ+=	audio_format_mp3.o
CFLAGS+=	-DBUILD_MP3
LDFLAGS+=	-lmad -lid3tag
endif
ifndef NO_VORBIS
HERRIE_OBJ+=	audio_format_vorbis.o
CFLAGS+=	-DBUILD_VORBIS
LDFLAGS+=	-lvorbisfile -lvorbis -logg
endif
ifndef NO_SNDFILE
HERRIE_OBJ+=	audio_format_sndfile.o
CFLAGS+=	-DBUILD_SNDFILE
LDFLAGS+=	-lsndfile
endif
ifndef NO_SCROBBLER
HERRIE_OBJ+=	scrobbler.o scrobbler_hash.o scrobbler_send.o
CFLAGS+=	$(shell curl-config --cflags) -DBUILD_SCROBBLER
LDFLAGS+=	$(shell curl-config --libs) -lssl
endif
ifndef NO_TRANS
CFLAGS+=	-DBUILD_TRANS -DTRANSDIR=\"$(TRANSDIR)\"
LOCALES=	nl
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

# libao dies with -Wstrict-prototypes - fixed in trunk
CFLAGS+=	$(shell pkg-config --cflags glib-2.0 gthread-2.0) \
		-I$(PREFIX)/include -DAUDIO_OUTPUT=\"$(AUDIO_OUTPUT)\" \
		-DAPP_NAME=\"herrie\" -DAPP_VERSION=\"$(VERSION)\" \
		-Wall -Werror -Wmissing-prototypes -include src/stdinc.h
LDFLAGS+=	$(shell pkg-config --libs glib-2.0 gthread-2.0) \
		-L$(PREFIX)/lib

HERRIE_OBJ+=	audio.o audio_output_$(AUDIO_OUTPUT).o config.o \
		gui_browser.o gui_draw.o gui_input.o gui_msgbar.o \
		gui_playq.o gui_vfslist.o main.o playq.o vfs.o \
		vfs_dir.o vfs_file.o vfs_playlist.o

.PHONY: all install uninstall clean depends

all: herrie herrie.1 $(LOCALES:%=%.mo)

install: all
	install -s herrie $(PREFIX)/bin/herrie$(BINARY_EXT)
	install -m 444 herrie.1 $(MANDIR)/man1/herrie.1
	$(foreach lang,$(LOCALES), \
		mkdir -p $(TRANSDIR)/$(lang)/LC_MESSAGES)
	$(foreach lang,$(LOCALES), \
		install -m 444 $(lang).mo \
		    $(TRANSDIR)/$(lang)/LC_MESSAGES/herrie.mo)

clean:
	rm -f herrie *.o *.mo *.core

%.mo: %.po
	msgfmt -o $@ $<

herrie: $(HERRIE_OBJ)

include depends.mak
