# Apolo: make Lua launcher scripts

Apolo is a library to aid the creation of Lua launcher scripts, replacing
Shell scripts and Windows batch files.

## Introduction

### Launcher scripts

Launcher scripts are script files that are used by some programs to adjust
the environment and control their execution. These scripts are usually
written in Bash on Linux, and as .BAT files on Windows -- cross-platform
programs will have to write both.

To avoid the problems of Bash and Windows batch files, many people choose
other scripting languages for the task, like Perl, Python or Ruby. The problem
with these choices is that, when the platform doesn't support them natively
(i.e. Windows), the developer is forced to provide the interpreter together
with their program -- which can be a problem, since the interpreters for these
languages are several megabytes in size.

### Why Lua

It's already possible to write launcher scripts in Lua today, since Lua is
just as much of a general-purpose programming language as the ones mentioned
above. Also, for most applications, with a size of a few hundred kilobytes,
the weight a Lua interpreter will put on the distribution of a program is
negligible, making it a very good fit in that regard.

However, pure Lua lacks some core functionalities needed for launcher scripts,
making it necessary for the developer to select a set of libraries that suits
their purpose (e.g. filesystem manupilation); also, many convenience features
present in Bash and the languages above are not present in Lua. So, while it's
_possible_ to write these kinds of scripts in Lua, it is not as _convenient_
as it is in the languages above.

### Goal

The final goal of the lirbary is making Lua about as convenient to write
launcher scripts as, say, Python or Perl, by removing the need to select a set
of necessary variables for the task and providing an API with similar
functionalities.

## Compililing

Apolo is a **Lua 5.3** C library, so the Lua headers need to be available.
It has been tested with **gcc** under both Windows (through MinGW[1]) and
Linux. To compile, run `make` from the root directory, choosing the platform.
The available platforms are `linux` and `mingw`:

    make linux

[1]: http://mingw.org/

Other compilers might work (like Clang or MSVC), but they haven't been tested.

### Deployment

This library is part a pure Lua library, part a C Lua library. The easiest way
to deploy an application using a Lua script with Apolo is to drop the Lua
interpreter, the script itself, and `apolo.lua` plus `apolocore.{so|dll}` in
the root dir of the application.

## Usage

To use Apolo in a Lua script, just require the library directly:

    require 'apolo'

Since the goal of Apolo is to make scripts whose primary purpose is to launch
programs, meaning that its functions will be the main functionality of the
script, all of the will be available globally by default. A planned feature is
to provide the option of loading the library as it is usually done for most
libraries -- that is:

    # Not implemented yet
    local apolo = require 'apolo'

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

### `E`

Access the system environment. You can access the environment variables by
using them as if they were fields of the `E` table -- for example, `E.HOME`,
`E.SHELL`, etc.:

    require 'apolo'

    run{E.CC, 'hello.c', '-o hello'}

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
