all:
	# Please choose your platform

linux:
	$(CC) lib/apolocore.c lib/apolocore.linux.c -std=gnu99 -fPIC -shared \
	-o lib/apolocore.so -I$(LUA_INCDIR) $(C_FLAGS) -Wall -Wextra

mingw:
	mingw32-gcc lib/apolocore.c lib/apolocore.win.c -std=gnu99 -fPIC -shared \
	-o lib/apolocore.dll -I$(LUA_DIR)/src -L$(LUA_DIR)/src -llua53 -Wall -Wextra

clean:
	rm lib/apolocore.so
