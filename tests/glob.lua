local apolo = require 'apolo'

apolo.dir.mk('globtests', function()
    -- Start patterns
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

    -- Some more star patterns
    local globi = apolo.glob('*i*')

    assert(
        #globi == 6,
        'Could not get all files with the letter i: ' ..
        apolo.inspect(globi))

    apolo.writef('cadu.h', '// oi')

    local globcd = apolo.glob('c*d*.h')
    assert(#globcd == 2, 'Could not get c*d*.c glob')

    -- optional patterns
    apolo.writef('codes.c', '// more codes')

    local globcodes = apolo.glob('code?.c')
    assert(
        #globcodes == 2,
        'Could not glob code.c and codes.c: ' .. apolo.inspect(globcodes))

    -- Char option test
    local globcode2 = apolo.glob('code[._]*')
    assert(
        #globcode2 == 4,
        'Could not match pattern code[._]: ' .. apolo.inspect(globcode2))
end)

apolo.del('globtests')
