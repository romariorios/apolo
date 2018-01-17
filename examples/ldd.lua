require 'apolo':as_global()

local textdomain = "libc"
local textdomaindir = "/usr/share/locale"

local rtldlist = {
    "/lib/ld-linux.so.2",
    "/lib64/ld-linux-x86-64.so.2",
    "/libx32/ld-linux-x32.so.2"
}

local opts, parse_err = parseopts{
    named = {
        version = {type = "switch"},
        help = {type = "switch"},
        ["data-relocs"] = {type = "switch"},
        ["function-relocs"] = {type = "switch"},
        verbose = {type = "switch"},
        unused = {type = "switch"}
    },
    multi_positional = "files"
}

if not opts then
    io.stderr:write("ldd: " .. parse_err .. "\n")
    io.stderr:write("Try `ldd --help' for more information.\n")
    os.exit(1)
end

if opts.version then
    print("ldd (Debian GLIBC 2.24-11+deb9u1) 2.24")
    print(string.format([[Copyright (C) %s Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
]], "2016"))
    print(string.format("Written by %s and %s", "Roland McGrath", "Ulrich Drepper"))
    os.exit()
end

if opts.help then
    print([[Usage: ldd [OPTION]... FILE...
    --help              print this help and exit
    --version           print version information and exit
-d, --data-relocs       process data relocations
-r, --function-relocs   process data and function relocations
-u, --unused            print unused direct dependencies
-v, --verbose           print all information
]])
    print(string.format("For bug reporting instructions, please see:\n%s.\n",
        "<http://www.debian.org/Bugs/>"))
    os.exit()
end

local warn = (opts["data-relocs"] or opts["function-relocs"]) and "yes" or nil
local bind_now = opts["function-relocs"] and "yes" or nil

local function nonelf()
    -- Maybe extra code for non-ELF binaries
    return 1
end

local add_env = {LD_TRACE_LOADED_OBJECTS = 1, LD_WARN = warn, LD_BIND_NOW = bind_now}
add_env.LD_LIBRARY_VERSION = "$verify_out"
add_env.LD_VERBOSE = opts.verbose and "yes" or nil

if opts.unused then
    local ld_debug = E.LD_DEBUG
    if ld_debug then
        ld_debug = ld_debug .. ",unused"
    else
        ld_debug = "unused"
    end

    add_env.LD_DEBUG = "\"" .. ld_debug .. "\""
end

-- The following command substitution is needed to make ldd work in SELinux
-- environments where the RTLD might not have permission to write to the
-- terminal.  The extra "x" character prevents the shell from trimming trailing
-- newlines from command substitution results.  This function is defined as a
-- subshell compound list (using "(...)") to prevent parameter assignments from
-- affecting the calling shell execution environment.

local function try_trace()
    -- TODO try_trace
end

local single_file = false

if not opts.files then
    io.stderr:write("ldd: missing file arguments\n")
    io.stderr:write("Try `ldd --help' for more information.\n")
    os.exit(1)
elseif #opts.files == 1 then
    single_file = true
end

for _, file in ipairs(opts.files) do
    if not single_file then
        print(file .. ":")
    end

    -- TODO
end
