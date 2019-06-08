require 'apolo':as_global()

local ps = nil

if core.osname == 'linux' then
    ps = '/'
elseif core.osname == 'win' then
    ps = '\\'
end

chdir('..' .. ps .. 'tests' .. ps)
local testlist = readf('testlist')
local matches = string.gmatch(testlist, '%S+')

for t in matches do
    -- Run tests with lua executable
    run{arg[-1], t .. '.lua'}
end

