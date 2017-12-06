apolo_global = true
require 'apolo'

dir.mk('apoloruntests', function()
    print('This test is supposed to run the "dir" command')

    for i = 1, 10 do
        writef('file_' .. i, 'These are the contents of this file: CONTENT')
    end

    print('These files should be shown: ' .. inspect(dir.entries()))
    run{'dir'}
end)

del('apoloruntests')

assert(
    not run{'non-existent'},
    'Should return false on non-existent executable')

local testsdir = string.gsub(arg[0], '(.*)/(.*)', '%1')
local luacmd = arg[-1]

assert(dir(testsdir, function()
    writef(
        'run-args.lua',
        [[
            for i, v in ipairs(arg) do
                io.write(i .. ' ' .. v .. ' ')
            end
            io.write('\n')
        ]])

    assert(run{luacmd, 'run-args.lua', 'one-arg'})
    assert(run{luacmd, 'run-args.lua', 'this-arg', 'another-arg', 'yet another arg'})
    assert(run{luacmd, 'run-args.lua', 'HELLO this is just one arg', 'another arg'})
end))

dir.mk('exitcodetests', function()
    writef(
        'exit-code.lua',
        [[
            if arg[1] == 'success' then
                os.exit()
            end

            os.exit(10)
        ]])

    assert(run{luacmd, 'exit-code.lua', 'success'})
    assert(run{luacmd, 'exit-code.lua'} == false)

    local ret, errstr = run{'luna', 'exit-code.lua', 'success'}
    assert(not ret)
    assert(errstr == 'Command not found')
end)

del('exitcodetests')
