PREFIX ?= /usr/local
LINUX_LUA_LIBNAME ?= lua5.3
LINUX_FLAGS = -DAPOLO_OS_LINUX -std=gnu99 -I$(LUA_INCDIR) -L$(LUA_LIBDIR) $(C_FLAGS) -Wall -Wextra
MINGW_FLAGS = -DAPOLO_OS_WIN -std=gnu99 -I$(LUA_DIR)\src -L$(LUA_DIR)\src $(C_FLAGS) -lshlwapi \
-Wall -Wextra
LINUX_LIBSRCS = lib/apolocore.c lib/apolocore.linux.c
WIN_LIBSRCS = lib/apolocore.c lib/apolocore.win.c

all:
	# Please choose your platform

linux:
	$(CC) $(LINUX_LIBSRCS) $(LINUX_FLAGS) -fPIC -shared -o lib/apolocore.so
	$(CC) launcher/gen_apolo_lua.c -o gen_apolo_lua && ./gen_apolo_lua
	rm -f gen_apolo_lua
	$(CC) $(LINUX_LIBSRCS) $(LINUX_FLAGS) launcher/main.c -o apolo \
		-l:lib$(LINUX_LUA_LIBNAME).a -lm -ldl

mingw:
	mingw32-gcc $(WIN_LIBSRCS) $(MINGW_FLAGS) -llua53 -fPIC -shared -o lib/apolocore.dll
	mingw32-gcc launcher/gen_apolo_lua.c -o gen_apolo_lua.exe && gen_apolo_lua.exe
	del gen_apolo_lua.exe
	mingw32-gcc $(WIN_LIBSRCS) $(MINGW_FLAGS) launcher/main.c $(LUA_DIR)\src\liblua.a -o apolo.exe

install:
	mkdir -p $(PREFIX)/share/lua/5.3
	install lib/apolo.lua $(PREFIX)/share/lua/5.3
	mkdir -p $(PREFIX)/lib/lua/5.3
	install lib/apolocore.so $(PREFIX)/lib/lua/5.3

clean:
	rm -f lib/apolocore.so
	rm -f launcher/apolo_lua.h
	rm -f apolo
