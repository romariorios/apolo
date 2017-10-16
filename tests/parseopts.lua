apolo_global = true
require 'apolo'

local result, error = parseopts{
    named = {
        verbose = {type = "switch"},
        version = {type = "switch"},
        somevalue = {type = "param"}
    },
    positional = {"thing1", "thing2"},
    multi_positional = "many-things"
}

if not result then
    print(error)
else
    for k, v in pairs(result) do
        print(k, v)
    end
end
