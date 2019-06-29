require 'apolo':as_global()

print("Your current username should be shown:")
run 'whoami'

del('apoloruntests')

assert(
    not run 'non-existent',
    'Should return false on non-existent executable')

local luacmd = arg[-1]

chdir.mk('argstests', function()
    writef(
        'run-args.lua',
        [[
            print("In run-args")
            for i, v in ipairs(arg) do
                io.write(i .. ' ' .. v .. ' ')
            end
            io.write('\n')
        ]])
    run 'dir'
    
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

del('envtests')

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

del('modifiertests')

print("Your current username should be shown:")
print(eval 'whoami')

del('apoloevaltests')

assert(
    not eval 'non-existent',
    'Should return false on non-existent executable')

chdir.mk('argstests', function()
    writef(
        'eval-args.lua',
        [[
            local out = ""
            for i, v in ipairs(arg) do
                out = out .. i .. ' ' .. v .. ' '
            end
            io.write(out)
        ]])

    assert(eval{luacmd, 'eval-args.lua', 'one-arg'} == "1 one-arg ")
    assert(eval{luacmd, 'eval-args.lua', 'this-arg', 'another-arg', 'yet another arg'}
            == "1 this-arg 2 another-arg 3 yet another arg ")
    assert(eval{luacmd, 'eval-args.lua', 'HELLO this is just one arg', 'another arg'}
            == "1 HELLO this is just one arg 2 another arg ")
    assert(eval(luacmd .. ' eval-args.lua "HELLO this is just one arg" and these are other args')
            == "1 HELLO this is just one arg 2 and 3 these 4 are 5 other 6 args ")
    assert(eval(luacmd .. " eval-args.lua 'just one arg' and other args")
            == "1 just one arg 2 and 3 other 4 args ")
end)

del('argstests')

chdir.mk('envtests', function()
    writef(
        'check-env.lua',
        [[
            if os.getenv('FOO') == 'hello' and os.getenv('BAR') == 'hey' then
                os.exit(3)
            end

            if os.getenv('FOO') == 'foo' then
                io.write("a")
            elseif os.getenv('BAR') == 'bar' then
                io.write("b")
            end

            os.exit(0)
        ]])

    assert(eval.env{FOO = 'foo'}{luacmd, 'check-env.lua'} == "a", "env modifer for eval failed")
    assert(eval.env{BAR = 'bar'}{luacmd, 'check-env.lua'} == "b", "env modifer for eval failed")
end)

del('envtests')