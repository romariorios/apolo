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