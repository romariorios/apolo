require 'apolo':as_global()

chdir.mk('apoloruntests', function()
    print('This test is supposed to run the "dir" command')

    for i = 1, 10 do
        writef('file_' .. i, 'These are the contents of this file: CONTENT')
    end

    print('These files should be shown: ' .. inspect(entries()))
    run 'dir'
end)

del('apoloruntests')

assert(
    not run 'non-existent',
    'Should return false on non-existent executable')

local luacmd = arg[-1]

chdir.mk('argstests', function()
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

    assert(run(
        luacmd ..
        ' run-args.lua "HELLO this is just one arg" and these are other args'))

    assert(run(luacmd .. " run-args.lua 'just one arg' and other args"))
end)

del('argstests')

chdir.mk('exitcodetests', function()
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

chdir.mk('envtests', function()
    writef(
        'check-env.lua',
        [[
            if os.getenv('FOO') == 'hello' and os.getenv('BAR') == 'hey' then
                os.exit(3)
            end

            if os.getenv('FOO') == 'foo' then
                os.exit(1)
            elseif os.getenv('BAR') == 'bar' then
                os.exit(2)
            end

            os.exit(0)
        ]])

    local _, exit_code = run.env{FOO = 'foo'}{luacmd, 'check-env.lua'}
    assert(exit_code == 1)

    local _, exit_code = run.env{BAR = 'bar'}{luacmd, 'check-env.lua'}
    assert(exit_code == 2)

    local _, exit_code = run.env{FOO = 'hello', BAR = 'hey'}{luacmd, 'check-env.lua'}
    assert(exit_code == 3)

    assert(run{luacmd, 'check-env.lua'})
end)

chdir.mk('modifiertests', function()
    writef(
        'hello.lua',
        [[
            local f = io.open('hello', 'w')
            f:write(os.getenv('HELLO'))
            f:close()
        ]])

    run.bg.env{HELLO = 'hello'}{luacmd, 'hello.lua'}
    run 'sleep 1'

    assert(readf('hello'), 'hello')
end)
