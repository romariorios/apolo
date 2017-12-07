require 'apolo'

del('copymovetests')

dir.mk('copymovetests', function()
    local file1text = 'This is file 1'

    writef('file1', file1text)
    copy('file1', 'file1copy')

    assert(
        readf('file1copy') == file1text,
        'file1copy text: ' .. readf('file1copy'))
    assert(exists('file1'))

    move('file1', 'file1moved')
    assert(
        readf('file1moved') == file1text,
        'file1moved text: ' .. readf('file1moved'))
    assert(not exists('file1'))

    for i = 1, 10 do
        local num = i * 5

        writef(
            'filey' .. num,
            'filey text')
    end

    dir.mk 'copied_files'
    dir '..'

    assert(copy(glob('filey*'), 'copied_files'))
    dir('copied_files', function()
        for _, v in ipairs(dir.entries()) do
            assert(readf(v) == 'filey text', 'Got text: ' .. readf(v))
        end
    end)

    dir.mk 'moved_files'
    dir '..'

    local cur_entryinfos = dir.entryinfos()
    assert(move(glob('filey*'), 'moved_files'))
    for _, f in ipairs(glob('filey*')) do
        assert(not cur_entryinfos[f], 'File ' .. f .. ' was not moved')
    end
end)

del('copymovetests')
