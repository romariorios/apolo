/* Copyright (C) 2021 Luiz Romário Santana Rios

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>

#define OPEN_FILE(__varname, __filename, __mode) \
    FILE* __varname = fopen(__filename, __mode); \
    if (!__varname) { \
        fprintf(stderr, "Failure to open %s\n", __filename); \
        return 1; \
    }

int main(void)
{
    OPEN_FILE(apolo_lua, "lib/apolo.lua", "r");
    OPEN_FILE(apolo_lua_h, "launcher/apolo_lua.h", "w");

    fprintf(apolo_lua_h, "const char* apolo_lua =\n\"");

    int c;
    while ((c = fgetc(apolo_lua)) != EOF) {
	if ((char) c == '\n') {
	    fprintf(apolo_lua_h, "\\n\"\n\"");
	    continue;
	}

	if ((char) c == '"' || (char) c == '\\')
	    fputc('\\', apolo_lua_h);
	fputc(c, apolo_lua_h);
    }

    fprintf(apolo_lua_h, "\";");

    fclose(apolo_lua_h);
    fclose(apolo_lua);
}
