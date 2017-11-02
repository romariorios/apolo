linux: linux-apologui
	${CC} lib/apolocore.c lib/apolocore.linux.c -std=gnu99 -fPIC -shared \
	-o lib/apolocore.so -I${LUA_INCDIR} ${C_FLAGS} -Wall -Wextra

GTK_LIBS:=$(shell pkg-config --cflags --libs gtk+-2.0)

linux-apologui:
	${CC} lib/apolocoregui.c lib/apolocoregui.linux.c -std=gnu99 -fPIC -shared \
	-o lib/apolocoregui.so -I${LUA_INCDIR} ${C_FLAGS} ${GTK_LIBS}

clean:
	rm lib/apolocore.so
	rm lib/apolocoregui.so
