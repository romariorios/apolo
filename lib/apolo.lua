-- Copyright (C) 2017--2019 Luiz Romário Santana Rios
-- Copyright (C) 2019 Connor McPherson
--
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the "Software"),
-- to deal in the Software without restriction, including without limitation
-- the rights to use, copy, modify, merge, publish, distribute, sublicense,
-- and/or sell copies of the Software, and to permit persons to whom the
-- Software is furnished to do so, subject to the following conditions:

-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.

-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
-- THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
-- DEALINGS IN THE SOFTWARE.

local apolo = {}

apolo.core = require 'apolocore'

-- Only windows needs a native call to RemoveDirectory
if apolo.core.osname ~= 'win' then
    apolo.core.rmdir = os.remove
end

if apolo.core.osname == 'linux' then
    function apolo.core.copy(orig, dest)
        return apolo.run{'/bin/cp', '-r', '--preserve=all', '--', orig, dest}
    end

    function apolo.core.move(orig, dest)
        return apolo.run{'/bin/mv', '--', orig, dest}
    end
end

local path_sep
if apolo.core.osname == 'win' then
    path_sep = '\\'
else
    path_sep = '/'
end

function apolo.abspath(path)
    if path:sub(1, 1) == path_sep then
        return path
    end

    if apolo.core.osname == 'win' and path:sub(2, 3) == ':\\' then
        return path
    end

    return apolo.current() .. path_sep .. path
end

function apolo:as_global()
    -- Declare everything in _ENV
    for k, v in pairs(self) do
        _ENV[k] = v
    end
end

local function make_apolo_command(options, option_types, call)
    local apolo_command = {}
    local apolo_command_mt = {}

    function apolo_command_mt.__index(_, option)
        local new_options = {}
        for k, v in pairs(options) do new_options[k] = v end

        local opttype = option_types[option]
        if opttype == 'switch' then
            new_options[option] = true
            return make_apolo_command(new_options, option_types, call)
        elseif opttype == 'param' then
            return function(value)
                new_options[option] = value
                return make_apolo_command(new_options, option_types, call)
            end
        else
            error('Unknown option "' .. option .. '"')
        end
    end
    
    function apolo_command_mt.__call(_, ...)    
        return call(options, ...)
    end

    setmetatable(apolo_command, apolo_command_mt)
    return apolo_command
end

apolo.chdir = {}

function apolo.chdir.mk(dir, fun)
    apolo.mkdir(dir)

    return apolo.chdir(dir, fun)
end

local apolo_chdir_mt = {}

function apolo_chdir_mt.__call(apolo_dir, dir, fun)
    -- Get old dir
    local old_dir = apolo.core.curdir()

    -- Enter dir
    assert(apolo.core.chdir(dir), "Could not enter directory \"" .. dir .. "\"")

    -- If no function was specified, then remain in the given dir
    if not fun then
        return true
    end

    fun()
    apolo.core.chdir(old_dir)

    return true
end

setmetatable(apolo.chdir, apolo_chdir_mt)

local function apolo_copymove(orig, dest, coreop)
    assert(type(dest) == 'string', 'Expecting string as destination')

    if type(orig) == 'string' then
        local success = coreop(orig, dest)

        return success
    end

    assert(type(orig) == 'table', 'Expecting string or table as origin')

    for _, path in ipairs(orig) do
        if not coreop(path, dest) then
            return false, 'Failed on file ' .. path
        end
    end

    return true
end

function apolo.copy(orig, dest)
    return apolo_copymove(orig, dest, apolo.core.copy)
end

function apolo.move(orig, dest)
    return apolo_copymove(orig, dest, apolo.core.move)
end

apolo.current = apolo.core.curdir

apolo.currentos = {}
apolo.currentos.linux = apolo.core.osname == 'linux'
apolo.currentos.win = apolo.core.osname == 'win'
apolo.currentos.name = apolo.core.osname

function apolo.del(entry)
    local cur_entry_infos = apolo.entry_infos()[entry]
    if not cur_entry_infos then
        return false
    end

    if cur_entry_infos.type == 'dir' then
        -- Enter dir and remove everything in it
        apolo.chdir(entry, function()
            for name, e in pairs(apolo.entry_infos()) do
                if e.type == "dir" then
                    assert(apolo.del(name))
                else
                    assert(os.remove(name))
                end
            end
        end)

        -- Then, finally, try removing the empty directory
        assert(apolo.core.rmdir(entry))
    else
        assert(os.remove(entry))
    end

    return true
end

apolo.E = {}
local apolo_E_mt = {}

function apolo_E_mt.__index(_, ind)
    return os.getenv(ind)
end

setmetatable(apolo.E, apolo_E_mt)

function apolo.entries(dir)
    local infos = apolo.entry_infos(dir)
    local res = {}

    -- Get only names
    for name, _ in pairs(infos) do
        table.insert(res, name)
    end

    return res
end

function apolo.entry_infos(dir)
    local dir = dir and dir or '.'
    local raw = apolo.core.listdirentries(dir)
    local res = {}

    -- Exclude . and .. from listing
    for _, e in ipairs(raw) do
        if e.name ~= '.' and e.name ~= '..' then
            local name = e.name

            e.name = nil
            res[name] = e
        end
    end

    return res
end

apolo.exists = apolo.core.exists

local function apolo_inspect(value, visited)
    local vtype = type(value)
    local res = ""

    if vtype == "table" then
        res = res .. "{"

        local cur_key = 1
        local continuous_array = true

        for k, v in pairs(value) do
            -- Check if key is a number and if the indexes are "continuous"
            if type(k) == "number" and continuous_array and k ~= cur_key then
                continuous_array = false
            else
                cur_key = cur_key + 1
            end

            -- Only show key if it's not a number
            if type(k) == "string" then
                res = res .. k .. " = "
            elseif type(k) ~= "number" or not continuous_array then
                res = res .. "[" .. apolo.inspect(k) .. "] = "
            end

            local was_visited = false
            for _, e in ipairs(visited) do
                if e == v then
                    was_visited = true
                end
            end

            table.insert(visited, value)
            local to_string = was_visited and '...' or apolo_inspect(v, visited)

            res = res .. to_string .. ", "
        end

        res = string.sub(res, 1, #res - 2) .. "}"
    elseif vtype == "string" then
        res = "\"" .. value .. "\""
    else
        res = tostring(value)
    end

    return res
end

function apolo.inspect(value)
    return apolo_inspect(value, {})
end

apolo.jobs = {}

local apolo_jobs_mt = {}

--Job call function that prints all still existing processes created by .bg
function apolo_jobs_mt.__call(apolo_jobs, status_filter)
    if status_filter then
        job_table = {}
        for pid, proc in pairs(apolo_jobs) do
            local status = proc:status()
            if status == status_filter then
                table.insert(job_table, proc)
            end
        end
        return job_table
    end
    return apolo_jobs
end

setmetatable(apolo.jobs, apolo_jobs_mt)

function apolo.path(...)
    local res = ''
    local items = {...}
    for i, item in ipairs(items) do
        res = res .. item
        if i ~= #items then -- do not append path_sep if last item
            res = res .. path_sep
        end
    end

    return res
end

local apolo_proc_mt = {}
function apolo_proc_mt.kill(self)
    return apolo.core.job_kill(self.pid, true)
end
function apolo_proc_mt.terminate(self)
    return apolo.core.job_kill(self.pid, false)
end

local function apolo_check_job(self, is_wait)
    local status, exit_code = apolo.core.job_status(self.pid, is_wait)
    self.cur_status = status or self.cur_status
    
    if exit_code ~= nil and exit_code < 0 then exit_code = nil end
    
    if status == "finished" or status == "failed" then
        self.status = function(self) return status end
        self.exit_code = function(self) return exit_code end
    end

    return exit_code
end

function apolo_proc_mt.wait(self)
    return apolo_check_job(self, true)
end
function apolo_proc_mt.status(self)
    apolo_check_job(self, false)
    return self.cur_status
end
function apolo_proc_mt.exit_code(self)
    return apolo_check_job(self, false)
end
function apolo_proc_mt.suspend(self)
    apolo_check_job(self, false)
    if self.cur_status == "running" then
        return apolo.core.job_active(self.pid, true)
    else
        return nil, "process can not be suspended because it is not running"
    end
end
function apolo_proc_mt.resume(self)
    apolo_check_job(self, false)
    if self.cur_status == "suspended" then
        return apolo.core.job_active(self.pid, false)
    else
        return nil, "process is not suspended"
    end
end

local function apolo_matches_glob_pattern(pattern, str)
    local pat = {}

    -- Parse the pattern before using it
    local i = 1
    local patlen = #pattern
    while i <= patlen do
        local ch = string.sub(pattern, i, i)

        if ch == '*' then
            table.insert(pat, '*')
            i = i + 1
        elseif ch == '?' then
            table.insert(pat, '?')
            i = i + 1
        elseif ch == '[' then
            local j = i + 1
            local subch = string.sub(pattern, j, j)
            local charopts = {'opts'}

            while subch ~= ']' do
                table.insert(charopts, subch)
                j = j + 1
                subch = string.sub(pattern, j, j)
            end

            table.insert(pat, charopts)
            i = j + 1
        else
            table.insert(pat, ch)
            i = i + 1
        end
    end

    -- Now use the parsed pat to see if str matches
    local pati = 1  -- Pattern index
    for i = 1, #str do
        local ch = string.sub(str, i, i)
        local curpatchar = pat[pati]

        -- If there's no more pattern left, but there are still characters,
        -- the globbing fails
        if not curpatchar then
            return false
        end

        if curpatchar == '*' then
            -- Keep consuming str's chars until ch is equal to next patchar
            if ch == pat[pati + 1] then
                pati = pati + 2
            end
        elseif curpatchar == '?' then
            -- Consume one char then go to the next patchar
            if ch == pat[pati + 1] then
                pati = pati + 2
            else
                pati = pati + 1
            end
        elseif type(curpatchar) == 'string' then
            -- Stop comparing and return false
            if ch ~= curpatchar then
                return false
            end

            pati = pati + 1
        else
            curpatchar[0] = nil  -- Remove opts

            -- Check if the current char corresponds to any of the options
            matches = false
            for _, opt in pairs(curpatchar) do
                if ch == opt then
                    matches = true
                    break
                end
            end

            if not matches then
                return false
            end

            pati = pati + 1
        end
    end

    -- If the pattern was not completely cosumed, then there wasn't a match
    return pati >= #pat
end

function apolo.glob(pattern)
    local res = {}

    for _, e in ipairs(apolo.entries()) do
        if apolo_matches_glob_pattern(pattern, e) then
            table.insert(res, e)
        end
    end

    return res
end

apolo.mkdir = apolo.core.mkdir

local function merge_lists(...)
    local result = {}
    for _, l in pairs{...} do
        if l then
            for _, e in ipairs(l) do
                table.insert(result, e)
            end
        end
    end

    return result
end        

function apolo.parseopts(options)
    local named = options.named
    local positional = options.positional
    local multi_positional = options.multi_positional
    local pos_index = 1
    local results = {}

    -- Flatten named switch and param names
    local named_names = merge_lists(named.switches, named.params)

    -- TODO Deal with short options

    -- Skip arguments
    local skip_next = false
    for arg_index, a in ipairs(arg) do
        if skip_next then
            skip_next = false
        elseif a == "--" then
            break
        elseif string.sub(a, 1, 2) == "--" then
            local trimmed = string.sub(a, 3, #a)
            local matches = nil

            -- For each prefix of an argument, check if it is unambiguous
            for i = 1, #trimmed do
                local prefix = string.sub(trimmed, 1, i)
                matches = {}

                -- Collect matches for this prefix
                for _, n in ipairs(named_names) do
                    local nprefix = string.sub(n, 1, i)

                    if prefix == nprefix then
                        table.insert(matches, n)
                    end
                end

                -- If there are no matches for a given prefix, there's no need
                -- to keep looking
                if #matches == 0 then
                    break
                end
            end

            -- If there's more than one match, then the option is ambiguous
            if #matches > 1 then
                local error_str =
                    "Option \"" .. a .. "\" is ambiguous (possible options: "

                -- List all possible matches for the prefix
                for i, m in ipairs(matches) do
                    error_str = error_str .. "--" .. m
                    if i ~= #matches then
                        error_str = error_str .. ", "
                    else
                        error_str = error_str .. ")"
                    end
                end

                return nil, error_str

            -- If there are no matches, then the option was not found
            elseif #matches == 0 then
                return nil, "Option \"" .. a .. "\" was not found"
            end

            -- Otherwise, there's only one match
            local match = matches[1]
            results[match] = true

            -- If option is param, get parameter value
            local match_is_param = false
            for _, p in ipairs(named.params or {}) do
                if match == p then
                    match_is_param = true
                    break
                end
            end

            if match_is_param then
                local error_str =
                    "Missing value for \"--" .. match .. "\" (" .. a .. ")"
                local next_arg = arg[arg_index + 1]

                -- If there's no next arg, return, since there must be a value
                -- for this option
                if not next_arg then
                    return nil, error_str
                end

                -- Check if this option has a value next to it in the cmdline
                if not (string.sub(next_arg, 1, 1) == "-" or
                        string.sub(next_arg, 1, 2) == "--") then

                    results[match] = next_arg
                    skip_next = true

                -- If there's no value next to this option, return
                else
                    return nil, error_str
                end
            end

        -- Otherwise, it's just a positional parameter
        else
            -- First, check unary positinal parameters
            if positional and pos_index <= #positional then
                results[positional[pos_index]] = a
                pos_index = pos_index + 1

            -- If there are no remaining unary positional params, check if
            -- there's a multi positional param
            else
                if not multi_positional then
                    return nil, "Unexpected argument \"" .. a .. "\""
                end

                if not results[multi_positional] then
                    results[multi_positional] = {}
                end

                table.insert(results[multi_positional], a)
            end
        end
    end

    return results
end

apolo.readf = {}

-- TODO support non-local protocols
apolo.readf.protocol_handlers = {}

local apolo_readf_mt = {}

function apolo_readf_mt.__call(apolo_readf, filename)
    -- Look for a colon in the matches and get the text before it
    local protocol, matches = string.gsub(filename, ":.*", "")

    -- If there's a match, then it's not a local file
    if matches >= 1 then
        -- Look for the right handler for the protocol
        for p, handler in pairs(apolo_readf.protocol_handlers) do
            if protocol == p then
                return handler(filename)
            end
        end

        return nil, "Unsupported protocol: " .. protocol
    end

    -- Otherwise, just open it as a normal local file
    local file, file_err = io.open(filename, "r")
    if not file then
        return nil, "Could not open file: " .. file_err
    end

    local file_str = file:read("*all")
    file:close()

    return file_str
end

setmetatable(apolo.readf, apolo_readf_mt)

local function unstringfy_args(str)
    local m = string.gmatch(str, '%S+')

    local args = {}
    local quoted = false
    local quote_char = nil

    for arg in m do
        if not quoted then
            quote_char = string.sub(arg, 1, 1)

            if quote_char == "'" or quote_char == '"' then
                quoted = true
                arg = string.sub(arg, 2, #arg)
            end

            table.insert(args, arg)
        else
            local arglen = #arg
            if string.sub(arg, arglen, arglen) == quote_char then
                quoted = false
                arg = string.sub(arg, 1, arglen - 1)
            end

            local argslen = #args
            args[argslen] = args[argslen] .. ' ' .. arg
        end
    end

    assert(quoted == false, 'Unclosed quote on command ' .. str)

    return args
end

local function make_apolo_command(options, option_types, call)
    local apolo_command = {}
    local apolo_command_mt = {}

    function apolo_command_mt.__index(_, option)
        local new_options = {}
        for k, v in pairs(options) do new_options[k] = v end

        local opttype = option_types[option]
        if opttype == 'switch' then
            new_options[option] = true
            return make_apolo_command(new_options, option_types, call)
        elseif opttype == 'param' then
            return function(value)
                new_options[option] = value
                return make_apolo_command(new_options, option_types, call)
            end
        else
            error('Unknown option "' .. option .. '"')
        end
    end
    
    function apolo_command_mt.__call(_, ...)
        return call(options, {...})
    end

    setmetatable(apolo_command, apolo_command_mt)
    return apolo_command
end

local function apolo_execute_call(options, args)
    assert(options.pipe or #args == 1, "Non-piped run commands must have only one command. "
        .. "Did you remember to encapsulate the command into a table?")
    assert(not options.pipe or #args > 1, "Piped run commands must have more than one command. "
        .. "Did you accidentally encapsulate all the commands in a table?")
    assert(#args < 32, "Maxed out pipe length. run.pipe has a limit of 32 processes")

    local exec_commands = {}

    -- Build exe commands by combining the executable and the args
    for _, val in ipairs(args) do
        local arg_table = {}
    
        if type(val) == 'string' then
            arg_table = unstringfy_args(val)
        else
            assert(
                type(val) == 'table',
                'Expecting either string or table as run argument')
            arg_table = val
        end
        
        table.insert(exec_commands, arg_table)
    end

    local envstrings = {}
    local exeenv = options.env
    if exeenv then
        for name, val in pairs(exeenv) do
            -- Convert lua booleans to a more usual representation of booleans
            -- used in command-line arguments and environment variables
            if val == true then
                val = 1
            elseif val == false then
                val = 0
            end

            table.insert(envstrings, name .. "=" .. tostring(val))
        end
    end

    -- Set warning for when users try appending to the same file twice at the same time
    if options.append_err_to == options.append_to and options.append_err_to ~= nil then
        print("WARNING!!! You are appending to same file multiple times simultaneously. " ..
            "This is not supported for all platforms and can cause file corruption on Linux.")
    end

    local result, errcode = apolo.core.execute(
        exec_commands, envstrings, options.bg, options.is_eval, #exec_commands,
        options.from or "", options.out_to or options.append_to or "",
        (options.out_to == nil), options.err_to or options.append_err_to or "",
        (options.err_to == nil), options.err_to_out, options.out_to_err)

    if result and options.bg  then
        -- Create process object
        local proc = {pid = result, name = executable, cur_status = "running"}
        jobs[proc.pid] = proc

        setmetatable(proc, {__index=apolo_proc_mt})

        return proc
    end

    if options.is_eval then
        return result
    else
        return result, errcode
    end
end

local apolo_run_options = {bg = 'switch', env = 'param', pipe = 'switch', from = 'param',
    out_to = 'param', append_to = 'param', err_to = 'param', append_err_to = 'param',
    err_to_out = 'switch', out_to_err = 'switch'}
apolo.run = make_apolo_command({bg = false, is_eval = false, err_to_out = false, out_to_err = false},
    apolo_run_options, apolo_execute_call)

local apolo_eval_options = {env = 'param', pipe = 'switch', from = 'param', err_to = 'param',
    append_err_to = 'param', err_to_out = 'switch'}
apolo.eval = make_apolo_command({bg = false, is_eval = true, err_to_out = false, out_to_err = false},
    apolo_eval_options, apolo_execute_call)

apolo.writef = {}

local function apolo_writef(filename, content, mode)
    local file, file_err = io.open(filename, mode)
    if not file then
        return nil, "Could not open file: " .. file_err
    end

    local _, write_err = file:write(content)
    if write_err then
        return nil, "Could not write to file: " .. write_err
    end

    assert(file:close())
    return true
end

function apolo.writef.app(filename, content)
    return apolo_writef(filename, content, "a")
end

local apolo_writef_mt = {}

function apolo_writef_mt.__call(_, filename, content)
    return apolo_writef(filename, content, "w")
end

setmetatable(apolo.writef, apolo_writef_mt)


return apolo
