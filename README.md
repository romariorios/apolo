# Apolo: make Lua launcher scripts

Apolo is a library with the goal of aiding the creation of Lua launcher scripts,
replacing shell script and Windows batch. It offers some functionalties that
standard Lua lacks, like filesystem manipulation mechanisms, for example. It
also offers some facilities that can be (and are) written in pure Lua, but
are repeatedly used in a context of launcher scripts and are more convenient
than using pure lua directly (for example, reading files driectly to strings
without exposing the file handle).

There are many libraries that offer the functionalties provided by Apolo
currently, but the difference is that Apolo is just one library with two files
aiming to 1) contain all functionalities and offer them in a way that makes
writing scripts in Lua as convenient as writing them in, say, Bash, and 2)
it aims to be multi-platform, so you can write the same initialization script
for both Windows and Linux.

It has the following functionalities implemented:

- Filesystem operations (copy, move, delete files; enter directories, etc.)
- Running commands with a custom environment
- Command-line options

## Usage

### Compililing

Apolo is currently tested on Linux and Windows -- using gcc and mingw,
respectively. To compile for Linux, run

    make linux

To compile for Windows:

    mingw32-make mingw

assuming `mingw32-make` is the command for Make in your system.

Apolo was tested on gcc on Linux, and on MinGW on Windows. Aside from that,
it depends only on Lua 5.3.

### Running

To run Apolo or include it in a Lua script, just require the library directly:

    require 'apolo'

All the functions (as well as all env vars) will be available globally. The
library is composed of the `apolo.lua` and `apolocore.so` or `apolocore.dll`
files -- both the lua library and the shared library need to be available as
imports in order for the library to work. One way to make it work, for example,
is just running lua from where the libraries are at.

### Running tests

To run all tests, first compile the project, then enter the `lib` directory and
execute the `runtests.lua` script -- which is itself written using Apolo.

## Library reference

These following are the functions available in the library and their usage.

### `copy(orig, dest)`

- Arguments:
  - `orig`: string or list of strings
  - `dest`: string
- Return: boolean

Copies `orig` to `dest`; returns `false` in case of an error. If `orig` is a
list of files, `copy` will return `false` on the first error and will abort
the copy.

### `del(entry)`

- Arguments:
  - `entry`: string
- Return: boolean

Deletes the file or directory `entry` and returns `true`. If the file doesn't
exist, returns `false`. If this function fails to delete `entry`, it raises
an error.

### `dir[.mk](d[, f])`

- Arguments:
  - `d`: string
  - `f`: function
- Return: boolean

When called as `dir(d)`, changes the current directory to `d` and returns
`false` in case of failure. Calling it as `dir.mk(d)` will create the
directory `d` if it doesn't already exist.

If a function `f` is passed, the current directory won't be changed and `f`
will be executed in `d` instead.

### `dir.entries([d])`

- Arguments:
  - `d`: string
- Return: list of strings

Returns a list with all entry names (files and directories) in `d`. If a
directory isn't passed as an argument, the function will return the entries
of the current directory.

### `dir.entryinfos([d])`

- Arguments:
  - `d`: string
- Return: table

Returns a table with the entry names of `d` as keys and tables containing
information about these entries as values. If a directory isn't passed as
an argument, the function will return the entries of the current directory.
The following information is available:

- `type`: Entry type
  - `dir`: directory
  - `file`: file
  - `unknown`: Apolo doesn't know what this entry is (probably a bug in Apolo)
  - `blkdev`: block device (Linux-only)
  - `chrdev`: character device (Linux-only)
  - `namedpipe`: named pipe (Linux-only)
  - `symlink`: symbolic link (Linux-only)
  - `udsocket`: Unix domain socket (Linux-only)

### `exists(path)`

- Arguments:
  - `path`: string
- Return : boolean

Returns `true` if `path` exists; `false` otherwise.

### `inspect(value)`

- Arguments:
  - `value`: any Lua value
- Return: string

Returns a string representation of `value`. The string representation is not
guaranteed to be correct Lua code; instead, the function tries to make the
most natural-looking literal for the value -- which can be valid Lua code for
simple values.

### `glob(pattern)`

- Arguments:
  - `pattern`: string
- Return: list of strings

Gets a list of entries in the current directory that match `pattern`.

### `move(orig, dest)`

- Arguments:
  - `orig`: string or list of strings
  - `dest`: string
- Return: boolean

Moves `orig` to `dest`; returns `false` in case of an error. If `orig` is a
list of files, `move` will return `false` on the first error and will abort
the copy.

### `parseopts(options)`

- Arguments:
  - `options`: table
- Return: table

Receives a table describing how to parse the command-line options and returns
a table contaning the parsed options from the command-line. The table has the
following fields:

- `named`: table contaning named command-line paramaters and switches
- `positional`: list containing positional command-line parameters
- `multi_positional`: string defining the name of a multi-positional argument

The `named` table has the name of the options as its keys and a table
with a single field named `type` that defines if the option is a `param` or a
`switch` (a `param` expects a value next to it in the command-line, while a
`switch` does not).

E.g.:

    require 'apolo'

    local opts = parseopts{
        named = {
            foo = {type = 'param'},
            bar = {type = 'param'},
            verbose = {type = 'switch'},
            help = {type = 'switch'}
        },
        positional = {'oof', 'rab'},
        multi_positional = 'files'
    }

The table `parseopts` returns contains all parameters that were present in the
command-line. Named switches will be attributed a `true` value if they were
present and `nil` otherwise; everything else will be assigned its value.

E.g. (continued from the previous code snippet:

    -- lua script.lua --foo 10 --verbose 20 30 40 50
    assert(opts.foo == '10')
    assert(not opts.bar)
    assert(opts.verbose)
    assert(not opts.help)
    assert(opts.oof == 20)
    assert(opts.rab == 30)

    local files = {'40', '50'}
    for i = 1, #files do
        assert(opts.files[i] == files[i])
    end

It's also possible to abbreviate command-line options, as long as they're
unambiguous -- for example, `--ver` instead of `--verbose`.

### `readf(filename)`

- Arguments:
  - `filename`: string
- Return: string

Returns the contents of `filename` as a string. On failure, it returns `nil`,
followed by the error string.

It's possible to install protocol handlers in the `readf.protocol_handlers` table.
For example, if you want `readf` to handle `http` addresses, do the following:

    require 'apolo'

    function readf.protocol_handlers.http(url)
        -- code to handle http addresses
    end

    local robots_txt = readf 'http://duckduckgo.com/robots.txt'

### `run[.env(env_table)](command)`

- Arguments:
  - `env_table`: table
  - `command`: string or table
- Return:
  - On success: boolean, number
  - On failure: nil, string

Runs processes. If `command` is a string, it will be parsed and executed.
Otherwise, if it's a table, the first element of `command` will be the
executable and all other elements will be the parameters:

    require 'apolo'

    -- equivalent commands
    run 'ls -la "foo bar"'
    run{'ls', '-la', 'foo bar'}

Executing the command as `run.env` will run the command with the current
environment plus the variables defined in `env_table`:

    run.env{LC_ALL = 'en_US'} 'ls -la "foo bar"'

It's also possible to store the return of `run.env` and have a new `run`-like
function that will execute all commands in your defined custom environment:

    local en_run = run.env{LC_ALL = 'en_US'}
    en_run 'ls -la "foo bar"'

It returns `true` followed by the exit code when it's successful; otherwise,
it returns `nil` followed by the error string. That way, the user can wrap
any run call with `assert`:

    assert(en_run 'lls -la "foo bar"')  -- Error: Command not found

### `writef[.app](filename, content)`

- Arguments:
  - `filename`: string
  - `content`: string

Write `content` to `filename`. If the file doesn't exist, it's created and
written into.

Otherwise, if `filename` exists, there are two options:

- `writef(filename, content)` will replace `filename`'s contents with `content`
- `writef.app(filename, content)` will append `content` to `filename`
