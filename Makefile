linux:
	${CC} lib/apolocore.c lib/apolocore.linux.c -fPIC -shared \
	-o lib/apolocore.so -I${LUA_INCDIR} ${C_FLAGS} -Wall -Wextra

clean:
	rm lib/apolocore.so
