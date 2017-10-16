apolo_global = true
require 'apolo'

local result, error = parseopts{
    named = {verbose = '', version = ''}
}

if not result then
    print(error)
else
    for k, v in pairs(result) do
        print(k, v)
    end
end
