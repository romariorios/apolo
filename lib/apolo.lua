-- Copyright (C) 2017 Luiz Romário Santana Rios
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

local apolo = nil
if apolo_global then
    apolo = _G
else
    apolo = {}
end

apolo.core = require 'apolocore'

setmetatable(_G, {
    __index = function(table, key)
        local v = rawget(table, key)
        if v then
            return v
        end

        return os.getenv(key)
    end})

apolo.dir = {}

function apolo.del(entry)
    -- If the entry can't be removed, then it's probably a directory
    if not os.remove(entry) then
        -- Enter dir and remove everything in it
        apolo.dir(entry, function()
            for _, e in ipairs(apolo.dir.entries()) do
                if e.type == "dir" then
                    assert(apolo.del(e.name))
                else
                    assert(os.remove(e.name))
                end
            end
        end)

        -- Then, finally, try removing the empty directory again
        assert(os.remove(entry))
    end

    return true
end

function apolo.dir.entries(dir)
    local dir = dir and dir or '.'
    local raw = apolo.core.listdirentries(dir)
    local res = {}

    -- Exclude . and .. from listing
    for _, e in ipairs(raw) do
        if e.name ~= '.' and e.name ~= '..' then
            table.insert(res, e)
        end
    end

    return res
end

function apolo.dir.mk(dir, fun)
    apolo.core.mkdir(dir)

    return apolo.dir(dir, fun)
end

local apolo_dir_mt = {}

function apolo_dir_mt.__call(apolo_dir, dir, fun)
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

setmetatable(apolo.dir, apolo_dir_mt)

function apolo.inspect(value)
    local vtype = type(value)
    local res = ""

    if vtype == "table" then
        -- TODO treat recursive tables

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

            res = res .. apolo.inspect(v) .. ", "
        end

        res = string.sub(res, 1, #res - 2) .. "}"
    elseif vtype == "string" then
        res = "\"" .. value .. "\""
    else
        res = tostring(value)
    end

    return res
end

function apolo.parseopts(options)
    local named = options.named
    local positional = options.positional
    local multi_positional = options.multi_positional
    local pos_index = 1
    local results = {}

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
                for n, _ in pairs(named) do
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

            -- Get option parameters
            local opt = named[match]
            local opttype = opt.type

            -- Type should be either switch or param
            assert(opttype,  "Missing type for \"" .. match .. "\" option")
            assert(
                opttype == "switch" or opttype == "param",
                "Type for \"" .. match ..
                "\" should be either \"param\" or \"switch\" (got \"" ..
                opttype .. "\")")

            -- If option is param, get parameter value
            if opttype == "param" then
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

apolo.read = {}

-- TODO support non-local protocols
apolo.read.protocol_handlers = {}

local apolo_read_mt = {}

function apolo_read_mt.__call(apolo_read, filename)
    -- Look for a colon in the matches and get the text before it
    local protocol, matches = string.gsub(filename, ":.*", "")

    -- If there's a match, then it's not a local file
    if matches >= 1 then
        -- Look for the right handler for the protocol
        for p, handler in pairs(apolo_read.protocol_handlers) do
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

setmetatable(apolo.read, apolo_read_mt)

function apolo.run(args)
    local executable = args[1]
    local exeargs = {}

    for i = 2, #args do
        table.insert(exeargs, args[i])
    end

    local envstrings = {}
    local exeenv = args.env
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

    return apolo.core.run(executable, exeargs, envstrings)
end

if not apolo_global then
    return apolo
end
