local apolo = require 'apolo'

print(apolo.inspect(apolo.dir.entries()))
apolo.dir('..', function()
    print(apolo.inspect(apolo.dir.entries()))
end)
print(apolo.inspect(apolo.dir.entries()))

-- Test del
apolo.dir.mk('tests', function()
    for i = 1, 10 do
        apolo.writef('file_' .. i, 'CONTENT')
    end
end)

apolo.del('tests')
