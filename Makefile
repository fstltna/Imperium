# Makefile for Imperium (Linux)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 1, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Usage:
#   make                    build everything
#   make install-user       create the impown account (as root, once)
#   make install            install into $(IMP_DEST) (usually needs root)
#   make install-systemd    install and load the systemd services
#   make clean              remove build output (keeps impsec.h)
#   make distclean          also remove impsec.h (the secret codes!)
#
# Any setting below can be changed on the command line, for example:
#   make install IMP_OWN=games IMP_GRP=games
#   make install DESTDIR=/tmp/pkgroot      (for building packages)

# =================== Settings you may want to change

# Where the game is installed
IMP_DEST ?= /usr/local/lib/imperium
# Where links to ConImp and TelImp are put
BIN_DEST ?= /usr/local/bin
# The account and group that own the installed game
IMP_OWN ?= impown
IMP_GRP ?= games
# Staging directory for packaging (normally empty)
DESTDIR ?=
# Where "make install-systemd" puts the unit files
SYSTEMD_DIR ?= /etc/systemd/system

# Base revision number shown by the game
IMP_BR = 1.5

# Debug output in the library ("Debug Enabled" in the version command).
# Set LIB_DEBUG= to turn it off. SERV_DEBUG=-DDEBUG_SERV adds server
# debugging.
LIB_DEBUG ?= -DDEBUG_LIB
SERV_DEBUG ?=

CC ?= cc
CFLAGS ?= -O2 -g
AR ?= ar
INSTALL ?= install

# =================== Nothing below here should normally need changing

# Always on: these catch mistakes that newer compilers refuse outright
WARNFLAGS = -Wall -Werror=implicit-function-declaration -Werror=implicit-int \
	-Werror=int-conversion -Werror=incompatible-pointer-types \
	-Werror=return-type
# -MMD -MP writes a .d file next to each .o listing the headers it used,
# so changing a header rebuilds everything that depends on it
DEPFLAGS = -MMD -MP
DEFS = -DIMP_BASE_REV='"$(IMP_BR)"' -DFIFO_DIR='"$(IMP_DEST)/fifo"'
ALL_CFLAGS = $(CPPFLAGS) $(DEFS) $(WARNFLAGS) $(DEPFLAGS) $(CFLAGS)
LIBS = -lcrypt

LIB_SRCS = $(addprefix Library/, messages.c util.c commands.c parse.c \
	startup.c scan.c cmd_edit.c cmd_user.c update.c cmd_general3.c \
	cmd_general1.c cmd_map.c cmd_general2.c cmd_naval.c cmd_move.c \
	cmd_naval2.c cmd_verify.c cmd_action.c cmd_trade.c cmd_telep.c \
	feSupp.c cmd_fight.c)
LIB_OBJS = $(LIB_SRCS:.c=.o)
IMPLIB = Library/libimpm.a

PROGS = Clients/ConImp Clients/TelImp Servers/ImpServ Servers/ImpShut \
	Servers/ImpCtrl ImpCre/ImpCre

ALL_OBJS = $(LIB_OBJS) Clients/ConImp.o Clients/TelImp.o \
	Servers/ServerMain.o Servers/FileIO.o Servers/ImpShut.o \
	Servers/ImpCtrl.o ImpCre/ImpCre.o

.PHONY: all clean distclean install install-check install-user \
	install-systemd sizes FORCE

all: $(PROGS)

# Settings like IMP_DEST are compiled into the programs, so remember what
# they were and rebuild everything if they change (for example if you run
# "make install IMP_DEST=/opt/imperium" after a plain "make").
BUILD_SETTINGS = $(CC) $(DEFS) $(CFLAGS) $(LIB_DEBUG) $(SERV_DEBUG)
.build-settings: FORCE
	@echo '$(BUILD_SETTINGS)' | cmp -s - $@ || echo '$(BUILD_SETTINGS)' > $@

$(ALL_OBJS): .build-settings

# The secret client/server codes. Made once and then kept, since every
# program (and the running server) has to use the same ones.
impsec.h:
	sh ./gSCodes

# Everything that talks to the server needs the codes
Clients/ConImp.o Clients/TelImp.o Servers/ServerMain.o Servers/ImpShut.o \
Servers/ImpCtrl.o: impsec.h

Library/%.o: Library/%.c
	$(CC) $(ALL_CFLAGS) $(LIB_DEBUG) -c -o $@ $<

Servers/%.o: Servers/%.c
	$(CC) $(ALL_CFLAGS) $(SERV_DEBUG) -c -o $@ $<

%.o: %.c
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

$(IMPLIB): $(LIB_OBJS)
	rm -f $@
	$(AR) rcs $@ $(LIB_OBJS)

Clients/ConImp: Clients/ConImp.o $(IMPLIB)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

Clients/TelImp: Clients/TelImp.o $(IMPLIB)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

Servers/ImpServ: Servers/ServerMain.o Servers/FileIO.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

Servers/ImpShut: Servers/ImpShut.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

Servers/ImpCtrl: Servers/ImpCtrl.o $(IMPLIB)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ImpCre/ImpCre: ImpCre/ImpCre.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

# Developer tools: print the sizes of the data structures
sizes: Include/ImpSize Include/ImpLibSize
Include/ImpSize: Include/ImpSize.c
	$(CC) $(CFLAGS) -o $@ $<
Include/ImpLibSize: Include/ImpLibSize.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) $(ALL_OBJS) $(ALL_OBJS:.o=.d) $(IMPLIB) \
		Include/ImpSize Include/ImpLibSize .build-settings \
		Servers/imperium-stop $(addprefix systemd/,$(UNITS))

distclean: clean
	rm -f impsec.h

# Make sure the owner/group exist before installing, rather than failing
# part way through with a confusing message from install(1)
install-check:
	@id -u $(IMP_OWN) >/dev/null 2>&1 || { \
	    echo "User '$(IMP_OWN)' does not exist. Create it first, e.g."; \
	    echo "    useradd --system --no-create-home $(IMP_OWN)"; \
	    echo "or install as another user: make install IMP_OWN=<user>"; \
	    exit 1; }
	@getent group $(IMP_GRP) >/dev/null 2>&1 || { \
	    echo "Group '$(IMP_GRP)' does not exist. Create it first, or use"; \
	    echo "    make install IMP_GRP=<group>"; \
	    exit 1; }

# Create the account the game runs as, if it isn't there already. It gets
# no login shell and no home directory of its own.
install-user:
	@getent group $(IMP_GRP) >/dev/null 2>&1 || groupadd --system $(IMP_GRP)
	@if id -u $(IMP_OWN) >/dev/null 2>&1; then \
	    echo "User $(IMP_OWN) already exists"; \
	else \
	    useradd --system --no-create-home --home-dir $(IMP_DEST) \
	        --shell /usr/sbin/nologin --gid $(IMP_GRP) $(IMP_OWN) && \
	    echo "Created user $(IMP_OWN)"; \
	fi

D = $(DESTDIR)$(IMP_DEST)
OWN = -o $(IMP_OWN) -g $(IMP_GRP)
# Fill in the install settings in the scripts and unit files
SUBST = sed -e 's|@IMP_DEST@|$(IMP_DEST)|g' -e 's|@IMP_OWN@|$(IMP_OWN)|g' \
	-e 's|@IMP_GRP@|$(IMP_GRP)|g'

install: all install-check
	$(INSTALL) -d $(OWN) -m 755 $(D) $(D)/clients $(D)/servers \
		$(D)/impcre $(D)/help $(D)/doc $(D)/data
	$(INSTALL) $(OWN) -m 751 Clients/ConImp Clients/TelImp \
		Clients/starttelimpscreen $(D)/clients
	$(INSTALL) $(OWN) -m 711 Servers/ImpCtrl Servers/ImpShut $(D)/servers
	$(INSTALL) $(OWN) -m 4700 Servers/ImpServ $(D)/servers
	$(INSTALL) $(OWN) -m 751 Servers/startimpscreen $(D)/servers
	$(SUBST) Servers/imperium-stop.in > Servers/imperium-stop
	$(INSTALL) $(OWN) -m 750 Servers/imperium-stop $(D)/servers
	$(INSTALL) $(OWN) -m 700 ImpCre/ImpCre ImpCre/default.sh \
		ImpCre/bigworld.sh $(D)/impcre
	@# don't overwrite access lists a deity may have edited
	for f in ic.allow is.allow; do \
	    if [ ! -f $(D)/$$f ]; then \
	        $(INSTALL) $(OWN) -m 644 Servers/$$f $(D)/$$f; \
	    else \
	        echo "Keeping existing $(D)/$$f"; \
	    fi; \
	done
	$(INSTALL) $(OWN) -m 444 Help/*.hlp $(D)/help
	$(INSTALL) $(OWN) -m 444 Doc/*.doc $(D)/doc
	@# Clients make their reply pipes in fifo/. The sticky bit (like
	@# /tmp) stops users deleting or replacing each other's pipes.
	$(INSTALL) -d $(OWN) -m 1777 $(D)/fifo
	@# Only the server's account may create the server port, so nobody
	@# else can put a fake server there.
	$(INSTALL) -d $(OWN) -m 755 $(D)/fifo/server
	$(INSTALL) -d -m 755 $(DESTDIR)$(BIN_DEST)
	ln -sf $(IMP_DEST)/clients/ConImp $(DESTDIR)$(BIN_DEST)/ConImp
	ln -sf $(IMP_DEST)/clients/TelImp $(DESTDIR)$(BIN_DEST)/TelImp

# systemd services: the server, the telnet front end, and an optional daily
# timer that publishes the game to the public directory (not enabled).
UNITS = imperium.service imperium-telnet.service imperium-publish.service

install-systemd: install
	$(INSTALL) -d -m 755 $(DESTDIR)$(SYSTEMD_DIR)
	for u in $(UNITS); do \
	    $(SUBST) systemd/$$u.in > systemd/$$u && \
	    $(INSTALL) -m 644 systemd/$$u $(DESTDIR)$(SYSTEMD_DIR)/$$u || exit 1; \
	done
	$(INSTALL) -m 644 systemd/imperium-publish.timer $(DESTDIR)$(SYSTEMD_DIR)
	@if [ -z "$(DESTDIR)" ] && command -v systemctl >/dev/null 2>&1; then \
	    systemctl daemon-reload 2>/dev/null || \
	        echo "(systemd isn't running - run 'systemctl daemon-reload' later)"; \
	    echo; \
	    echo "Installed. Create a world first (see INSTALL), then:"; \
	    echo "    systemctl enable --now imperium.service imperium-telnet.service"; \
	fi

-include $(ALL_OBJS:.o=.d)
