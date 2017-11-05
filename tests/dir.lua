local apolo = require 'apolo'

apolo.run{'dir'}
apolo.dir('..', function()
    apolo.run{'dir'}
end)
apolo.run{'dir'}

-- Test del
apolo.dir.mk('tests', function()
    for i = 1, 10 do
        apolo.writef('file_' .. i, 'CONTENT')
    end
end)

apolo.del('tests')
