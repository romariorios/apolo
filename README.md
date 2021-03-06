# Apolo: launcher scripts in Lua

Apolo is a library to aid the creation of launcher scripts in Lua, replacing
Shell scripts and Windows batch files.

## Introduction

### Launcher scripts

Launcher scripts are script files that are used by some programs to adjust
the environment and control their execution. These scripts are usually
written in Bash on Linux, and as .BAT files on Windows -- cross-platform
developers will have to write both versions.

To avoid the problems of Bash and Windows batch files, many people choose
other scripting languages for the task, like Perl, Python or Ruby. The problem
with these choices is that, when the platform doesn't support them natively
(i.e. Windows), the developer is forced to provide the interpreter together
with their program; since the interpreters for these
languages are several megabytes in size, this can be a problem.

### Why Lua

It's already possible to write launcher scripts in Lua today, since Lua is
just as much of a general-purpose programming language as the ones mentioned
above. Also, with a size of a few hundred kilobytes,
the weight a Lua interpreter will put on the distribution of a program is
negligible for most applications, making it a very good fit in that regard.

However, pure Lua lacks some core functionalities needed by launcher scripts,
making it necessary for the developer to select a set of libraries that suits
their purpose (e.g. filesystem manipulation); also, many convenience features
present in Bash and the languages above are not present in Lua. So, while it's
_possible_ to write these kinds of scripts in Lua, it is not as _convenient_
as it is in the languages above.

### Goal

The goal of the library is to make Lua about as convenient to write
launcher scripts as, say, Python or Perl, by removing the need to select a set
of necessary variables for the task and providing an API with similar
functionalities.

## Compiling

Apolo is a **Lua 5.3** C library, so, to compile the library, the Lua headers need to be available.
The code has been tested with **gcc** under both Windows (through MinGW[1]) and
Linux. Run `make <plat>` from the root directory, where `<plat>` can be `linux` or `mingw`.

[1]: http://mingw.org/

Compilation should work in other compilers like Clang or MSVC, but we haven't tested
them, so it may require some tweaking.

### Deployment

This library is written in Lua and C. The easiest way
to deploy an application using a Lua script with Apolo is to drop the Lua
interpreter, the script itself, and `apolo.lua` plus `apolocore.{so|dll}` in
the root dir of the application.

## Usage

To use Apolo in a Lua script, you can require the library normally:

    local apolo = require 'apolo'

However, since the goal of Apolo is to make scripts whose primary purpose is
to launch programs, one might want to have all functions available at the
global level. To do that, you can require Apolo in the following way:

    require 'apolo':as_global()

### Running tests

To run all tests, first compile the project, then enter the `lib` directory and
execute the `runtests.lua` script -- which is itself written using Apolo.

## Library reference

The following are the functions available in the library and their usage.

### `apolo:as_global()`

Populate `_ENV` with the apolo functions, as seen in the "Usage" section above.

Requiring Apolo with `as_global()` makes it possible to use the Apolo functions
without prefixing them with `apolo` -- or whatever prefix you chose:

    -- Usual library-like require
    local apolo = require 'apolo'

    apolo.mkdir(apolo.current() .. '/test')

    -- as_global require
    require 'apolo':as_global()

    mkdir(current() .. '/test')

### `apolo.chdir[.mk](d[, f])`

- Arguments:
  - `d`: directory
  - `f`: function
- Return: boolean

When called as `chdir(d)`, changes the current directory to `d` and returns
`false` in case of failure. Calling it as `chdir.mk(d)` will create the
directory `d` if it doesn't already exist.

If a function `f` is passed, the current directory won't be changed and `f`
will be executed in `d` instead.

### `apolo.copy(orig, dest)`

- Arguments:
  - `orig`: path or sequence of paths to file or directory
  - `dest`: path to directory
- Return: boolean

Copies `orig` to `dest`; returns `false` in case of an error. If `orig` is a
sequence of files, `copy` will return `false` on the first error and will abort
the copy. If `dest` does not exist, the `copy` will create it.

### `apolo.current()`

- Return: current directory

Returns the current directory.

### `apolo.del(entry)`

- Arguments:
  - `entry`: path to file or directory
- Return: boolean

Deletes the file or directory `entry` and returns `true`. If the file doesn't
exist, returns `false`. If this function fails to delete `entry`, it raises
an error.

### `apolo.E`

Access the system environment. You can access the environment variables by
using them as if they were fields of the `E` table -- for example, `E.HOME`,
`E.SHELL`, etc.:

    require 'apolo':as_global()

    print(E.HOME)

### `apolo.entries([d])`

- Arguments:
  - `d`: directory
- Return: sequence of entries

Returns a sequence with all entry names (files and directories) in `d`. If a
directory isn't passed as an argument, the function will return the entries
of the current directory.

### `apolo.entry_infos([d])`

- Arguments:
  - `d`: directory
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

### `apolo.exists(path)`

- Arguments:
  - `path`: file or directory
- Return : boolean

Returns `true` if `path` exists; `false` otherwise.

### `apolo.eval[.env(env_table)][.pipe][.from(filename)][.err_to_out](command, ...)`

- Arguments:
  - `env_table`: table
  - `command`: string or table
  - `...`: Many strings or tables, representing commands piped together
- Return:
  - string

Runs processes and returns any output that would normally go to console.
If `command` is a string, it will be parsed and executed. Otherwise, if
it's a table, the first element of `command` will be the
executable and all other elements will be the parameters:

    require 'apolo':as_global()

    -- equivalent commands
    local file_contents = eval 'ls -la "foo bar"'
    local file_contents = eval{'ls', '-la', 'foo bar'}

Executing the command as `eval.env` will run the command with the current
environment plus the variables defined in `env_table`:

    local file_contents = eval.env{LC_ALL = 'en_US'} 'ls -la "foo bar"'

It's also possible to store the return of `eval.env` and have a new `eval`-like
function that will execute all commands in your defined custom environment:

    local en_eval = eval.env{LC_ALL = 'en_US'}
    local file_contents = en_eval 'ls -la "foo bar"'

Executing the command as `eval.pipe` will accept many commands into the function's
arguments, which will be piped into each other similar to a bash script. For more details,
check the documentation for `run`.

Executing the command as `eval.from` will get the input from a file instead of from
stdin.

Executing the command as `eval.err_to_out` will return the value of the error stream along
with the value from stdout.

Eval returns the console output as a string when it's successful. Otherwise, it
returns `nil` followed by the error string. This way, the user can wrap any run
call with `assert`:

    local file_contents = assert(en_eval 'lls -la "foo bar"')  -- Error: Command not found

### `apolo.inspect(value)`

- Arguments:
  - `value`: any Lua value
- Return: string representation of the value

Returns a string representation of `value`. The string representation is not
guaranteed to be correct Lua code; instead, the function tries to make the
most natural-looking literal for the value -- which can be valid Lua code for
simple values.

### `apolo.jobs[.status(status_filter)]()`

- Arguments:
  - `status_filter`: string
- Return: table of running background processes

Returns a table of all background processes started by `apolo.run.bg`. Processes
are represented by tables

The process table has three methods that can be used on it:
  - `proc:wait()`: pauses the program until the process ends and returns
the exit code
  - `proc:suspend()`: suspends the process
  - `proc:resume()`: resumes the process
  - `proc:kill()`: kills the process similar to the SIGKILL signal
  - `proc:terminate()`: terminates the process similar to the SIGTERM signal
  - `proc:status()`: returns `running`, `suspended`, `failed` or `finished`
depending on process state
  - `proc:exit_code()`: returns the exit code of the process. The meaning of
the code is process-dependent

You can also use `proc.pid` or `proc.name` to get information about the process

Executing the command with the added `jobs.status(status_filter)` will return a
partial table of only processes with a status that matches the status_filter.

Options for status_filter are `running`, `suspended`, `failed` and `finished`

E.g.:

    require 'apolo':as_global()

    local proc = run.bg('name_of_some_background_process')
    local old_processes = jobs() -- returns a table with x processes running
    proc:kill() -- kills the process and returns true on success

Optionally, you can treat apolo.jobs like a table to get a specific process
using its PID.

    -- Pid is set to the process id of some process
    jobs[pid]:terminate() -- terminates the process with the given pid

### `apolo.glob(pattern)`

- Arguments:
  - `pattern`: string
- Return: sequence of entries (files or directories)

Gets a sequence of entries in the current directory that match `pattern`.

### `apolo.mkdir(path)`

- Arguments:
  - `path`: path to new directory
- Return: boolean

Tries to create a new directory. Returns `false` when it fails.

### `apolo.move(orig, dest)`

- Arguments:
  - `orig`: path or sequence of paths
  - `dest`: path
- Return: boolean

Moves `orig` to `dest`; returns `false` in case of an error. If `orig` is a
sequence of files, `move` will return `false` on the first error and will abort
the copy.

### `apolo.parseopts(options)`

- Arguments:
  - `options`: table
- Return: table

Receives a table describing how to parse the command-line options and returns
a table containing the parsed options from the command-line. The table has the
following fields:

- `named`: table containing named command-line parameters and switches
- `positional`: sequence containing positional command-line parameters
- `multi_positional`: string defining the name of a multi-positional argument

The `named` table can have the following fields:

- `switches`: sequence containing command-line switches
- `params`: sequence contaning command-line params

The difference between switches and params is that a `param` expects a value
next to it in the command-line, while a `switch` does not.

E.g.:

    require 'apolo':as_global()

    local opts = parseopts{
        named = {
            params = {'foo', 'bar'},
            switches = {'verbose', 'help'}
        },
        positional = {'oof', 'rab'},
        multi_positional = 'files'
    }

The table returned by `parseopts` contains all parameters that were present in
the command-line. Named switches will be attributed a `true` value if they were
present and `nil` otherwise; everything else will be assigned its value.

E.g. (continued from the previous code snippet):

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

### `apolo.readf(filename)`

- Arguments:
  - `filename`: file
- Return: file contents

Returns the contents of `filename` as a string. On failure, it returns `nil`,
followed by the error string.

It's possible to install protocol handlers in the `readf.protocol_handlers` table.
For example, if you want `readf` to handle `http` addresses, do the following:

    require 'apolo':as_global()

    function readf.protocol_handlers.http(url)
        -- code to handle http addresses
    end

    local robots_txt = readf 'http://duckduckgo.com/robots.txt'

### `apolo.run[.bg][.pipe][.env(env_table)][.from(filename)][.out_to(filename)][.append_to(filename)][.err_to(filename)][.append_err_to(filename)][.err_to_out](command, ...)`

- Arguments:
  - `env_table`: table
  - `filename`: string
  - `command`: string or table
  - `...`: Many strings or tables, representing commands piped together
- Return:
  - On success: boolean, number (exit code)
  - On failure: nil, string (error message)
  - If using `.bg` modifier: process object

Runs processes. If `command` is a string, it will be parsed and executed.
Otherwise, if it's a table, the first element of `command` will be the
executable and all other elements will be the parameters:

    require 'apolo':as_global()

    -- equivalent commands
    run 'ls -la "foo bar"'
    run{'ls', '-la', 'foo bar'}

Executing the command as `run.bg` will spawn the process in the background and
return a table for the resulting process. The process table has its own
functions and variables (see `jobs` for more info).

Executing the command as `run.env` will run the command with the current
environment plus the variables defined in `env_table`:

    run.env{LC_ALL = 'en_US'} 'ls -la "foo bar"'

It's also possible to store the return of `run.env` and have a new `run`-like
function that will execute all commands in your defined custom environment:

    local en_run = run.env{LC_ALL = 'en_US'}
    en_run 'ls -la "foo bar"'

Executing the command as `run.pipe` will accept many commands into the function's
arguments, which will be piped into each other similar to a bash script:

    run.pipe('ls -l', 'grep .txt', 'sort')

Similarly, the commands can be tables of arguments:

    run.pipe({'ls', '-l'}, 'grep .txt', {'sort'})

Executing the command as `run.to` will write the output to a file instead of to
stdout. If the file already exists, its contents will be overwritten.

Executing the command as `run.append_to` will append the output to a file instead of to
stdout. If the file doesn't exist, it will be created, and if it does exist the output
will be appended to the end of the file. If both `.to` and `.append_to` are used, the
program will default to using `.to` and overwrite the file.

Executing the command as `run.from` will get the input from a file instead of from
stdin.

Executing the command as `run.err_to` will write the error stream (stderr) to a file.
If the file already exists, its contents will be overwritten, just like `.to`.

Executing the command as `run.append_err_to` will append the error stream (stderr) to a file.
It works like `.append_to`.

Executing the command as `run.err_to_out` will append the error stream (stderr) of all processes
to the output stream (stdout). This modifier does not work in conjunction with `.err_to`
or `append_err_to`. Be careful when using this modifier on a pipe, as multiple piped processes
may write their errors to out at the same time.

Executing the command as `run.out_to_err` will append the output stream (stdout) of the process
(or the last process in the pipe, if there is one) to the error stream (stderr). Similar to
`err_to_out`, this modifier does not work with `out_to` but does work with `err_to`.

Run returns `true` followed by the exit code when it's successful; otherwise,
it returns `nil` followed by the error string. That way, the user can wrap
any run call with `assert`:

    assert(en_run 'lls -la "foo bar"')  -- Error: Command not found

### `apolo.writef[.app](filename, content)`

- Arguments:
  - `filename`: path to file
  - `content`: file contents

Write `content` to `filename`. If the file doesn't exist, it's created and
written into.

Otherwise, if `filename` exists, there are two options:

- `writef(filename, content)` will replace `filename`'s contents with `content`
- `writef.app(filename, content)` will append `content` to `filename`
