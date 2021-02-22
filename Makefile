PREFIX ?= /usr/local
LINUX_FLAGS = -DAPOLO_OS_LINUX -std=gnu99 -I$(LUA_INCDIR) -L$(LUA_LIBDIR) $(C_FLAGS) -Wall -Wextra

all:
	# Please choose your platform

linux:
	$(CC) lib/apolocore.c lib/apolocore.linux.c $(LINUX_FLAGS) -fPIC -shared -o lib/apolocore.so

	$(CC) launcher/gen_apolo_lua.c -o gen_apolo_lua && ./gen_apolo_lua
	rm -f gen_apolo_lua
	$(CC) launcher/main.c lib/apolocore.c lib/apolocore.linux.c $(LINUX_FLAGS) -o apolo -llua5.3

mingw:
	mingw32-gcc lib/apolocore.c lib/apolocore.win.c -DAPOLO_OS_WIN -std=gnu99 \
	-fPIC -shared -o lib/apolocore.dll -I$(LUA_DIR)/src -L$(LUA_DIR)/src -llua53 \
	-lshlwapi -Wall -Wextra

install:
	mkdir -p $(PREFIX)/share/lua/5.3
	install lib/apolo.lua $(PREFIX)/share/lua/5.3
	mkdir -p $(PREFIX)/lib/lua/5.3
	install lib/apolocore.so $(PREFIX)/lib/lua/5.3

clean:
	rm lib/apolocore.so
