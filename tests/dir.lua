local apolo = require 'apolo'

apolo.run{'dir'}
apolo.dir('..', function()
    apolo.run{'dir'}
end)
apolo.run{'dir'}
