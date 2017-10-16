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

function apolo.parseopts(options)
    local named = options.named
    local positional = options.pos
    local multi_positional = options.multipos
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
                -- TODO list possible matches
                return nil, "Option \"" .. a .. "\" is ambiguous"

            -- If there are no matches, then the option was not found
            elseif #matches == 0 then
                return nil, "Option \"" .. a .. "\" was not found"
            end

            -- Otherwise, there's only one match
            local match = matches[1]
            results[match] = true

            local next_arg = arg[arg_index + 1]
            if not next_arg then
                break
            end

            -- Check if this option has a value next to it in the cmdline
            if not (string.sub(next_arg, 1, 1) == "-" or
                    string.sub(next_arg, 1, 2) == "--") then

                results[match] = next_arg
                skip_next = true
            end
        end
    end

    return results
end

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
