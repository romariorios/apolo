require 'apolo':as_global()

if not exists 'cmake' then
    error 'cmake is not in PATH'
end

if not CXX then
    error 'No CXX defined'
end

local tests =
    if not TESTS then
        -- TODO glob
    else
        TESTS
    end

if not CXX_FLAGS and (CXX == 'g++' or CXX == 'clang++') then
    CXX_FLAGS = '-g -O0'
end

CXX_FLAGS = '-std=c++14 ' .. CXX_FLAGS

if not RUBBERBAND_SRC then
    RUBBERBAND_SRC = dir '..'
end

if exists.dir 'build' then
    rmdir 'build'
end

mkdir 'build'

if not exists.dir 'inst' then
    mkdir 'inst'
end

dir('build', function()
    run{
        'cmake',
        '-DCMAKE_INSTALL_PREFIX=../inst',
        '-DCMAKE_CXX_COMPILER=' .. CXX,
        '-DCMAKE_CXX_FLAGS=' .. CXX_FLAGS,
        '-DCMAKE_BUILD_TYPE=' .. BUILD_TYPE,
        '-G Ninja'}:and_then()  -- fail whole script if this fails
    run{'cmake', '--build', '.', '--target', 'install'}
end):and_then()

local rubberband_path = dir 'inst'

for t in spaced_list(tests) do
    local is_disabled = false
    for dis in spaced_list(readf('disabled')) do
        if t == dis then
            is_disabled = true
            break
        end
    end

    if not is_disabled then
        local test_name = t .. 'test-' .. CXX
        print('--- ' .. test_name .. ' ---')
        run{
            CXX,
            t .. 'test.cpp',
            '-lrubberbandcore',
            '-lmodloader',
            '-o ' .. test_name,
            CXX_FLAGS,
            '-I' .. rubberband_path .. 'include',
            '-I' .. rubberband_path .. 'include/rbb',
            '-L' .. rubberband_path .. 'lib',
            '-std=c++14'
        }:and_then()

        run{
            test_name,
            env = {
                LD_LIBRARY_PATH =
                    LD_LIBRARY_PATH .. ':' .. rubberband_path .. '/lib'}
            }:and_then()

        del(test_name)
    end
end
