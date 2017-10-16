apolo_global = true
require 'apolo'

-- Test 1: normal switches
local opts = {
    named = {
        verbose = {type = "switch"},
        version = {type = "switch"},
        veritas = {type = "switch"},
        apply = {type = "switch"},
        append = {type = "switch"}
    }
}

-- Test 1.1: non-ambiguous full args
arg = {"--verbose", "--apply", "--append"}
local optvals = assert(parseopts(opts))

assert(optvals.verbose, "--verbose unavailable")
assert(optvals.apply, "--apply unavailable")
assert(optvals.append, "--append unavailable")

assert(not optvals.version, "--version should be nil")
assert(not optvals.veritas, "--veritas should be nil")

-- Test 1.2: ambiguous prefixes
arg = {"--ver", "--app"}
local optvals, err = parseopts(opts)

assert(not optvals, "Parsing should have failed")
print("Success: " .. err)

arg = {"--ap", "--ver"}
local optvals, err = parseopts(opts)

assert(not optvals, "Parsing should have failed")
print("Success: " .. err)

-- Test 1.3: non-ambiguous prefixes
arg = {"--verb", "--veri", "--appe"}
local optvals = assert(parseopts(opts))

assert(optvals.verbose, "--verbose unavailable")
assert(optvals.veritas, "--veritas unavailable")
assert(optvals.append, "--append unavailable")

-- Test 2: switches and named parameters
local opts = {
    named = {
        append = {type = "param"},
        path = {type = "param"},
        verbose = {type = "switch"},
        version = {type = "switch"}
    }
}

-- Test 2.1: non-ambiguous full named params
arg = {"--append", "10", "--path", "hello"}
local optvals = assert(parseopts(opts))

assert(optvals.append == "10", "Could not get value of --append")
assert(optvals.path == "hello", "Could not get value of --path")

-- Test 2.2: full named params with missing value
arg = {"--append", "--path", "hello"}
local optvals, err = parseopts(opts)

assert(not optvals, "Parsing should have failed")
print("Success: " .. err)

-- Test 2.3: non-ammbiguous prefixes
arg = {"--app", "10", "--p", "hello"}
local optvals = assert(parseopts(opts))

assert(optvals.append == "10", "Could not get value of --app")
assert(optvals.path == "hello", "Could not get value of --path")

-- Test 3: Unary positional params
local opts = {
    named = {
        append = {type = "param"},
        verbose = {type = "switch"},
        version = {type = "switch"}
    },
    positional = {"foo", "bar", "baz"}
}

-- Test 3.1: After named options
arg = {"--app", "10", "--verbose", "foo", "bar", "baz"}
local optvals = assert(parseopts(opts))

assert(optvals.append == "10", "Could not get value of --app")
assert(optvals.verbose, "Could not check --verbose switch")
assert(optvals.foo == "foo", "Could not get foo param")
assert(optvals.bar == "bar", "Could not get bar param")
assert(optvals.baz == "baz", "Could not get baz param")

-- Test 3.2: Before named options
arg = {"foo", "bar", "--app", "10", "--verbose"}
local optvals = assert(parseopts(opts))

assert(optvals.foo == "foo", "Could not get foo param")
assert(optvals.bar == "bar", "Could not get bar param")
assert(not optvals.baz, "Should not get baz param")
assert(optvals.append == "10", "Could not get value of --app")
assert(optvals.verbose, "Could not check --verbose switch")

-- Test 3.3: In between named options
arg = {"foo", "--app", "10", "bar", "--verbose", "baz"}
local optvals = assert(parseopts(opts))

assert(optvals.foo == "foo", "Could not get foo param")
assert(optvals.append == "10", "Could not get value of --app")
assert(optvals.bar == "bar", "Could not get bar param")
assert(optvals.verbose, "Could not check --verbose switch")
assert(optvals.baz == "baz", "Could not get baz param")

-- Test 3.4: Extra positional parameters (should fail)
arg = {"foo", "bar", "baz", "unexpected"}
local optvals, err = parseopts(opts)

assert(not optvals, "Parsing should have failed (got " .. tostring(optvals) .. ")")
print("Success: " .. err)

-- Test 4: Multi-positional params
opts.multi_positional = "extra"

-- Test 4.1: Only positional arguments
arg = {"foo", "bar", "baz", "extra1", "extra2", "extra3"}
local optvals = assert(parseopts(opts))

assert(optvals.foo == "foo", "Could not get foo param")
assert(optvals.bar == "bar", "Could not get bar param")
assert(optvals.baz == "baz", "Could not get baz param")
assert(#optvals.extra == 3, "Wrong number of extra params")

for i = 1, 3 do
    assert(optvals.extra[i] == "extra" .. i, "Extra params in the wrong order")
end

-- Test 4.2: Positional and named arguments
arg = {"foo", "bar", "baz", "--app", "10", "extra1", "--verbose", "extra2"}
local optvals = assert(parseopts(opts))

assert(optvals.foo == "foo", "Could not get foo param")
assert(optvals.bar == "bar", "Could not get bar param")
assert(optvals.baz == "baz", "Could not get baz param")
assert(optvals.append == "10", "Could not get value of --app")
assert(optvals.extra[1] == "extra1", "Could not get extra param")
assert(optvals.verbose, "Could not check --verbose switch")
assert(optvals.extra[2] == "extra2", "Could not get extra param")

-- Test 5: Misc cmdline args features

-- Test 5.1: Ignore subsequent args with --
arg = {"foo", "bar", "--", "baz"}
local optvals = assert(parseopts(opts))

assert(optvals.foo == "foo", "Could not get foo param")
assert(optvals.bar == "bar", "Could not get bar param")
assert(not optvals.baz, "Should not get baz")

-- Test 5.2: Assign param values with =
arg = {"--app=10", "foo"}
local optvals = assert(parseopts(opts))

assert(optvals.append == "10", "Could not get value of --app")
assert(optvals.foo == "foo", "Could not get foo param")
