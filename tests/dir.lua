require 'apolo'

print(inspect(dir.entries()))
dir('..', function()
    print(inspect(apolo.dir.entries()))
end)
print(inspect(dir.entries()))

-- Test del
dir.mk('tests', function()
    for i = 1, 10 do
        writef('file_' .. i, 'CONTENT')
    end
end)

del('tests')
