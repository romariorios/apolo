local apolo = require 'apolo'

apolo.dir.mk('globtests', function()
    for i = 1, 10 do
        apolo.writef('test' .. i, 'This is the test number ' .. i)
    end

    for i = 1, 4 do
        apolo.writef('thing' .. i, 'This is the thing number ' .. i)
    end

    local globtest = apolo.glob('test*')
    local globthing = apolo.glob('thing*')

    assert(#globtest == 10, 'Did not glob all test* files created')
    assert(#globthing == 4, 'Did not glob all thing* files created')

    apolo.writef('code.c', '// this is some code')
    apolo.writef('code.h', '// this is a header')
    apolo.writef('code_internal_linux.c', '// this is the linux-specific code')
    apolo.writef('code_internal_win.c', '// this is the windows-specific code')

    local globcodec = apolo.glob('code*.c')
    local globcodeh = apolo.glob('code*.h')

    assert(
        #globcodec == 3,
        'Did not glob all C source code files (result: ' ..
        apolo.inspect(globcodec) .. ')')
    assert(#globcodeh == 1, 'Did not glob all C header files')
end)
