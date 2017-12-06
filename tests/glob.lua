require 'apolo'

dir.mk('globtests', function()
    -- Start patterns
    for i = 1, 10 do
        writef('test' .. i, 'This is the test number ' .. i)
    end

    for i = 1, 4 do
        writef('thing' .. i, 'This is the thing number ' .. i)
    end

    local globtest = glob('test*')
    local globthing = glob('thing*')

    assert(#globtest == 10, 'Did not glob all test* files created')
    assert(#globthing == 4, 'Did not glob all thing* files created')

    writef('code.c', '// this is some code')
    writef('code.h', '// this is a header')
    writef('code_internal_linux.c', '// this is the linux-specific code')
    writef('code_internal_win.c', '// this is the windows-specific code')

    local globcodec = glob('code*.c')
    local globcodeh = glob('code*.h')

    assert(
        #globcodec == 3,
        'Did not glob all C source code files (result: ' ..
        inspect(globcodec) .. ')')
    assert(#globcodeh == 1, 'Did not glob all C header files')

    -- Some more star patterns
    local globi = glob('*i*')

    assert(
        #globi == 6,
        'Could not get all files with the letter i: ' ..
        inspect(globi))

    writef('cadu.h', '// oi')

    local globcd = glob('c*d*.h')
    assert(#globcd == 2, 'Could not get c*d*.c glob')

    -- optional patterns
    writef('codes.c', '// more codes')

    local globcodes = glob('code?.c')
    assert(
        #globcodes == 2,
        'Could not glob code.c and codes.c: ' .. inspect(globcodes))

    -- Char option test
    local globcode2 = glob('code[._]*')
    assert(
        #globcode2 == 4,
        'Could not match pattern code[._]: ' .. inspect(globcode2))
end)

del('globtests')
