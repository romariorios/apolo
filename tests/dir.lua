require 'apolo':as_global()

print(inspect(entries()))
chdir('..', function()
    print(inspect(entries()))
end)
print(inspect(entries()))

-- Test del
chdir.mk('tests', function()
    for i = 1, 10 do
        writef('file_' .. i, 'CONTENT')
    end
end)

del('tests')
