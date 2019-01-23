all:
	# Please choose your platform

linux:
	$(CC) lib/apolocore.c lib/apolocore.linux.c -std=gnu99 -fPIC -shared \
	-o lib/apolocore.so -I$(LUA_INCDIR) -L$(LUA_LIBDIR) $(C_FLAGS) -Wall

mingw:
	mingw32-gcc lib/apolocore.c lib/apolocore.win.c -std=gnu99 -fPIC -shared \
	-o lib/apolocore.dll -I$(LUA_DIR)/src -L$(LUA_DIR)/src -llua53 -Wall -Wextra

clean:
	rm lib/apolocore.so
