require 'apolo':as_global()

chdir(path('..', 'tests'))
local testlist = readf('testlist')
local matches = string.gmatch(testlist, '%S+')

for t in matches do
    -- Run tests with lua executable
    run{arg[-1], t .. '.lua'}
end

