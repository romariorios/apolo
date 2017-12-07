require 'apolo'

dir.mk('copymovetests', function()
    local file1text = 'This is file 1'

    writef('file1', 'file1text')
    copy('file1', 'file1copy')
    assert(readf('file1copy') == file1text)
    assert(exists('file1'))

    move('file1', 'file1moved')
    assert(readf('file1moved') == file1text)
    assert(not exists('file1'))
end)
