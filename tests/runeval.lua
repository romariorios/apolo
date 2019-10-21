require 'apolo':as_global()

print("Your current username should be shown three times:")
run 'whoami'
run('whoami')
run{'whoami'}

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
    
    local proc = run.bg{luacmd, 'exit-code.lua', 'success'}
    print(proc)
    proc:wait()
    assert(proc:exit_code() == 0, 'background process has wrong exit code')

    proc = run.bg{luacmd, 'exit-code.lua'}
    proc:wait()
    assert(proc:exit_code() == 10)
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

    a = run.bg.env{HELLO = 'hello'}{luacmd, 'hello.lua'}
    a:wait()

    assert(readf('hello')=='hello', 'hello')
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

    assert(eval.env{FOO = 'foo'}{luacmd, 'check-env.lua'} == "a",
        "env modifer for eval failed")
    assert(eval.env{BAR = 'bar'}{luacmd, 'check-env.lua'} == "b",
        "env modifer for eval failed")
end)

del('envtests')

-- Function to get endline for OS
function newline()
    if core.osname == 'win' then
        return "\r\n"
    else
        return "\n"
    end
    
end

chdir.mk('pipetests', function()
    writef(
        'concat.lua',
        [[
            local var = io.read()
            print("Piped " .. var)
        ]])
    writef(
        'other_concat.lua',
        [[
            local var = io.read()
            print("I have " .. var)
        ]])

    writef(
        'seed.lua',
        [[
            print("Hello World")
        ]])

    if pcall(run.pipe, 'lua seed.lua') then
        assert(false, 'pipe function with only one function succeeds')
    end
    if pcall(eval.pipe, 'lua seed.lua') then
        assert(false, 'pipe function with only one function succeeds')
    end
    if pcall(run, 'lua seed.lua', 'lua concat.lua') then
        assert(false, 'non-piped function with more than one function succeeds')
    end
    if pcall(eval, 'lua seed.lua', 'lua concat.lua') then
        assert(false, 'non-piped function with more than one function succeeds')
    end
    
    assert(run.pipe('lua seed.lua', 'lua concat.lua'), "Run 2-level pipe")
    assert(run.pipe({'lua', 'seed.lua'}, {'lua', 'concat.lua'}), "Run 2-level pipe")
    assert(eval.pipe('lua seed.lua', 'lua concat.lua') == "Piped Hello World" .. newline(), "Eval 2-level pipe")
    assert(eval.pipe({'lua', 'seed.lua'}, {'lua', 'concat.lua'})== "Piped Hello World" .. newline(), "Eval 2-level pipe")

    assert(run.pipe('lua seed.lua', 'lua concat.lua', 'lua other_concat.lua'), "Run 3-level pipe")
    assert(run.pipe('lua seed.lua', {'lua', 'concat.lua'}, 'lua concat.lua'), "Run Mixed 3-level pipe")
    assert(run.pipe({'lua', 'seed.lua'}, {'lua', 'concat.lua'}, {'lua', 'concat.lua'}), "Run 3-level pipe")
    assert(eval.pipe('lua seed.lua', 'lua concat.lua', 'lua other_concat.lua') == "I have Piped Hello World" .. newline(), "Eval 3-level pipe")
    assert(eval.pipe('lua seed.lua', {'lua', 'concat.lua'}, 'lua concat.lua') == "Piped Piped Hello World" .. newline(), "Eval Mixed 3-level pipe")
    assert(eval.pipe({'lua', 'seed.lua'}, {'lua', 'concat.lua'}, {'lua', 'concat.lua'}) == "Piped Piped Hello World" .. newline(), "Eval 3-level pipe")

    assert(run.pipe('whoami', 'lua concat.lua', {'lua', 'other_concat.lua'}, {'lua', 'concat.lua'}, 'lua other_concat.lua'), "Running longer 5-level pipe")
end)

del('pipetests')

chdir.mk('redirectiontests', function()
    writef(
        'read_write.lua',
        [[
            local var = io.read()
            io.write("Hello " .. var)
        ]])
    writef(
        'read_pipe.lua',
        [[
            local var = io.read()
            print("Hello " .. var)
        ]])
    writef(
        'write.lua',
        [[
            io.write("Greetings!")
        ]])
    writef('input.txt', [[World!]])

    -- Test .from
    assert(run.from("input.txt"){luacmd, "read_write.lua"}, ".from doesn't work in run")
    print(eval.from("input.txt"){luacmd, "read_write.lua"})
    assert(eval.from("input.txt"){luacmd, "read_write.lua"} == "Hello World!", ".from doesn't work in eval")
    -- Test .out_to
    assert(run.out_to("Output.txt"){luacmd, "write.lua"}, ".out_to doesn't work in run")
    assert(readf("Output.txt") == "Greetings!", ".out_to doesn't work in run")
    -- Test .from and .out_to together
    assert(run.from("input.txt").out_to("Output2.txt"){luacmd, "read_write.lua"}, ".out_to and .from together doesn't work in run")
    assert(readf("Output2.txt") == "Hello World!", ".out_to and .from together doesn't work in run")
    -- Test .from and .out_to with piping in run
    assert(run.from("input.txt").out_to("Output3.txt").pipe({luacmd, "read_pipe.lua"}, {luacmd, "read_write.lua"}),
        "piping with .out_to and .from together doesn't work in run")
    assert(readf("Output3.txt") == "Hello Hello World!", ".out_to and .from together doesn't work in run")
    -- Test .from with piping in eval
    assert(eval.from("input.txt").pipe({luacmd, "read_pipe.lua"}, {luacmd, "read_write.lua"}) == "Hello Hello World!",
        "piping .out_to and .from together doesn't work in eval")
    -- Test .append_to
    assert(run.append_to("Output.txt"){luacmd, "write.lua"}, ".append_to doesn't work in run")
    assert(readf("Output.txt") == "Greetings!Greetings!", ".append_to doesn't work in run")
    -- Test .append_to in combination with .from
    assert(run.from("input.txt").append_to("Output2.txt"){luacmd, "read_write.lua"},
        ".append_to and .from together doesn't work in run")
    assert(readf("Output2.txt") == "Hello World!Hello World!", ".append_to and .from together doesn't work in run")


    -- Test .from and .out_to in a background process (add when merged in with process-management)
    --local proc = run.from("input.txt").out_to("Output4.txt").bg{luacmd, "read_write.lua")
    --proc:wait()
    --assert(readf("Output4.txt") == "Hello Hello World!", ".out_to and .from together don't work in background processes")
end)

del('redirectiontests')

chdir.mk('redirecterrortests', function()
    writef(
        'error.lua',
        [[
            if arg[1] == 'fail' then
                error("Expected Error")
            else
                io.write("Greetings!")
            end
        ]])
    writef(
        'print_error.lua',
        [[
            print("Hello World!")
            error(arg[1])
        ]])
    writef(
        'read_error.lua',
        [[
            local val = io.read()
            error(val)
        ]])
    writef(
        'seed.lua',
        [[
            print("From Output")
        ]])
    writef(
        'read_write.lua',
        [[
            local var = io.read()
            io.write("Expected " .. var)
        ]])
    writef('input.txt', [[Expected Error]])

    function get_first_line(input_str)
        for str in string.gmatch(input_str, "([^\r\n]+)") do
            return str
        end
    end
    local clock = os.clock
    function sleep(n)  -- seconds
       local t0 = clock()
       while clock() - t0 <= n do
       end
    end

    -- Test .err_to
    assert(not run.err_to("err.txt"){luacmd, "print_error.lua", "Expected Error!"},
        "errored process returns true")
    assert(get_first_line(readf("err.txt")) == luacmd .. ": print_error.lua:2: Expected Error!",
        ".err_to doesn't write to file properly")
    -- Test .err_to with .out_to
    assert(run.err_to("err2.txt").out_to("out.txt"){luacmd, "error.lua", "succeed"},
        "running lua process fails")
    assert(readf("err2.txt") == "", ".err_to writes to file even when there is no error")
    assert(readf("out.txt") == "Greetings!")

    assert(not run.err_to("err2.txt").out_to("out.txt"){luacmd, "error.lua", "fail"},
        "errored lua process returns true")
    assert(get_first_line(readf("err2.txt"))
        == luacmd .. ": error.lua:2: Expected Error", ".err_to writes incorrectly to file")
    assert(readf("out.txt") == "", "run.out_to doesn't wipe file when overwriting it")

    -- Test .append_err_to
    local prelen = string.len(readf("err2.txt"))
    assert(not run.append_err_to("err2.txt"){luacmd, "error.lua", "fail"},
        "errored lua process returns true")
    assert(string.len(readf("err2.txt")) > prelen, "Append_err doesn't add to file length")

    -- Test that .err_to overwrites file
    assert(not run.err_to("err2.txt"){luacmd, "print_error.lua", "Another Expected Error"},
        "errored lua process returns true")
    assert(get_first_line(readf("err2.txt"))
        == luacmd .. ": print_error.lua:2: Another Expected Error", ".err_to doesn't overwrite file")

    -- Test eval.err
    print("Testing error activation!")
    assert(get_first_line(eval{luacmd, "print_error.lua", "Expected Error"}) == "Hello World!")
    assert(get_first_line(eval.err_to_out{luacmd, "error.lua", "fail"}) ==
        luacmd .. ": error.lua:2: Expected Error")

    -- Test .from with eval.err
    assert(get_first_line(eval.from('input.txt').err_to_out{luacmd, "read_error.lua"})
        == luacmd .. ": read_error.lua:2: Expected Error")

    -- Test .out_to_err
    assert(run.out_to_err.err_to("err3.txt")({luacmd, "seed.lua"}))
    assert(get_first_line(readf("err3.txt")) == "From Output")

    -- Test .out_to_err with piping
    assert(run.pipe.out_to_err.err_to("err3.txt")({luacmd, "seed.lua"}, {luacmd, "read_write.lua"}))
    assert(get_first_line(readf("err3.txt")) == "Expected From Output")
end)

del('redirecterrortests')
chdir.mk('backgroundmanagementtests', function()
    writef(
        'sleep_15.lua',
        [[
            local clock = os.clock
            for i= 1,15 do
                local t0 = clock()
                while clock() - t0 <= 1 do end
            end
        ]])
    writef(
        'sleep_5.lua',
        [[
            local clock = os.clock
            for i= 1,5 do
                local t0 = clock()
                while clock() - t0 <= 1 do end
            end
        ]])
    writef(
        'sleep_1.lua',
        [[
            local clock = os.clock
            local t0 = clock()
            while clock() - t0 <= 1 do end
        ]])
    
        
    local clock = os.clock
    function sleep(n)  -- seconds
        local t0 = clock()
        while clock() - t0 <= n do end
    end

    function length(T)
        local count = 0
        for _ in pairs(T) do count = count + 1 end
        return count
    end

    print("Testing background processes...")
    local num_jobs = length(jobs())
    local num_running = length(jobs("running"))
    local num_finished = length(jobs("finished"))

    run.bg{luacmd, 'sleep_15.lua'}
    local proc = run.bg{luacmd, 'sleep_15.lua'}
    run.bg{luacmd, 'sleep_5.lua'}

    assert(length(jobs()) == num_jobs+3, 'Jobs shows all processes')
    assert(length(jobs("running")) == num_running+3, 'Filtering running jobs')
    assert(length(jobs("finished")) == num_finished, 'Filtering finished jobs')
    sleep(6)
    assert(length(jobs("running")) == num_running+2)
    assert(length(jobs("finished")) == num_finished+1)
    assert(length(jobs("finished")) == num_finished+1,
        "Check that finished procs stay in list after being checked")
    print("Waiting for background process...")
    proc:wait()
    sleep(1)
    assert(proc:status() == "finished")
    assert(proc:exit_code() == 0, "naturally finished process has invalid exit code")
    assert(length(jobs("running")) == num_running,
        'Filtering running jobs after all procs end v1')
    assert(length(jobs("finished")) == num_finished+3,
        'Filtering finished jobs after all procs end v1')

    print("Testing process termination...")
    proc = run.bg{luacmd, 'sleep_15.lua'}
    proc:kill()
    sleep(1) -- Wait for proc to terminate
    assert(proc:status() == "failed")
    assert(not proc:exit_code(), "finished process does not have exit code of nil")

    proc = run.bg{luacmd, 'sleep_15.lua'}
    proc:terminate()
    proc:status()
    sleep(1) -- Wait for proc to terminate
    assert(proc:status() == "failed")
    assert(not proc:exit_code(), "killed process does not have exit code of nil")

    --local cur_jobs = length(jobs("running"))
    --assert(not run.bg{'llua sleep_15.lua'}, 'False start succeeds')
    --assert(length(jobs("running")) == cur_jobs, 'False starts add to "running" processes')

    --Test for suspension
    print("Testing process suspension...")
    proc = run.bg{luacmd, 'sleep_5.lua'}
    sleep(1)
    proc:suspend()
    sleep(1)
    assert(proc:status() == "suspended", "Process not suspended")

    run{luacmd, 'sleep_5.lua'} -- Wait for proc to potentially fail
    assert(proc:status() == "suspended", "Process suspension does not suspend process")

    --Test continuing process
    proc:resume()
    sleep(1)
    assert(proc:status() == "running", "Process not continued")

    run{luacmd, 'sleep_5.lua'} -- Wait for proc to end
    assert(proc:status() == "finished", "Continued process never ends")
    
    --Test for referencing
    print("Testing process referencing...")
    proc = run.bg{luacmd, 'sleep_1.lua'}
    assert(jobs[proc.pid]:status() == "running", "Referring process through jobs[pid] doesn't work")
    run{luacmd, 'sleep_5.lua'} -- Wait for proc to end
    assert(jobs[proc.pid]:status() == "finished", "Referring process through jobs[pid] doesn't work")
end)

del('backgroundmanagementtests')