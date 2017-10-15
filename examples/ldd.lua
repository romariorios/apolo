local textdomain = "libc"
local textdomaindir = "/usr/share/locale"

local rtldlist = {
    "/lib/ld-linux.so.2",
    "/lib64/ld-linux-x86-64.so.2",
    "/libx32/ld-linux-x32.so.2"
}

local warn = nil
local bind_now = nil
local verbose = nil
local pos_args = {}

for _, a in ipairs(arg) do
    if a == "--vers" or a == "--versi" or a == "--versio" or a == "--version" then
        print("ldd (Debian GLIBC 2.24-11+deb9u1) 2.24")
        print(string.format([[Copyright (C) %s Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
]], "2016"))
        print(string.format("Written by %s and %s", "Roland McGrath", "Ulrich Drepper"))
        os.exit()
    elseif a == "--h" or a == "--he" or a == "--hel" or a == "--help" then
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
    elseif a == "-d" or a == "--d" or a == "--da" or a == "--dat" or a == "--data" or
            a == "--data-" or a == "--data-r" or a == "--data-re" or a == "--data-rel" or
            a == "--data-relo" or a == "--data-reloc" or a == "--data-relocs" then
        warn = "yes"
    elseif a == "-r" or a == "--f" or a == "--fu" or a == "--fun" or a == "--func" or
            a == "--funct" or a == "--functi" or a == "--functio" or a == "--function" or
            a == "--function-" or a == "--function-r" or a == "--function-re" or
            a == "--function-rel" or a == "--function-relo" or a == "--function-reloc" or
            a == "--function-relocs" then
        warn = "yes"
        bind_now = "yes"
    elseif a == "-v" or a == "--verb" or a == "--verbo" or a == "--verbos" or
            a == "--verbose" then
        verbose = "yes"
    elseif a == "-u" or a == "--u" or a == "--un" or a == "--unu" or a == "--unus" or
            a == "--unuse" or a == "--unused" then
        unused = true
    elseif a == "--v" or a == "--ve" or a == "--ver" then
        io.stderr:write("ldd: option `" .. a .. "' is ambiguous\n")
        os.exit(1)
    elseif a == "--" then
        break
    elseif string.sub(a, 1, 1) == "-" then
        io.stderr:write("ldd: unrecognized option `" .. a .. "'\n")
        io.stderr:write("Try `ldd --help' for more information.\n")
        os.exit(1)
    else
        table.insert(pos_args, a)
    end
end

local function nonelf()
    -- Maybe extra code for non-ELF binaries
    return 1
end

local add_env = {LD_TRACE_LOADED_OBJECTS = 1, LD_WARN = warn, LD_BIND_NOW = bind_now}
add_env.LD_LIBRARY_VERSION = "$verify_out"
add_env.LD_VERBOSE = verbose

if unused then
    local ld_debug = os.getenv("LD_DEBUG")
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

if #pos_args == 0 then
    io.stderr:write("ldd: missing file arguments\n")
    io.stderr:write("Try `ldd --help' for more information.\n")
    os.exit(1)
elseif #pos_args == 1 then
    single_file = true
end

for _, file in ipairs(pos_args) do
    if not single_file then
        print(file .. ":")
    end

    -- TODO
end
