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
        warn = true
    elseif a == "-r" or a == "--f" or a == "--fu" or a == "--fun" or a == "--func" or
            a == "--funct" or a == "--functi" or a == "--functio" or a == "--function" or
            a == "--function-" or a == "--function-r" or a == "--function-re" or
            a == "--function-rel" or a == "--function-relo" or a == "--function-reloc" or
            a == "--function-relocs" then
        warn = true
        bind_now = true
    elseif a == "-v" or a == "--verb" or a == "--verbo" or a == "--verbos" or
            a == "--verbose" then
        verbose = true
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
    end
end
