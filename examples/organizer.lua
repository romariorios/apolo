-- This script organizes files inside your Downloads folder into respective folders holding files of similar type

require 'apolo':as_global()

-- file types groups table with their respective extensions
local extensions = {
    documents = {
        '.pdf', '.txt', '.xlsx', '.pptx', '.docx', '.rtf'
    },
    videos = {
        '.mkv', '.mp4', '.avi', '.flv', '.webm'
    },
    compressed = {
        '.zip', '.rar', '.7z', '.iso'
    },
    images = {
        '.png', '.jpg', '.jpeg',
    },
    music = {
        '.mp3', '.flac', '.ogg'
    },
    programs = {
        '.exe', '.deb'
    }
}

-- command line arguments
local opts = assert(parseopts{
    named = {
        params = { 'dir' },
        switches = { 'here', 'help' }
    }
})

-- print help information
if opts.help then
    print([[

    Organizes files by group type. Defaults to "Downloads" directory.

    Options:

    --dir [DIRECTORY_PATH] : organize files in given directory
    -- here : organize current directory
    -- help : show this help message
    ]])
    return
end

-- select directory from cli args
if opts.here then
    selectedDir = current()
elseif opts.dir then
    selectedDir = opts.dir
else
    selectedDir = E.HOMEPATH and 'C:' .. E.HOMEPATH .. '/Downloads' or E.HOME .. '/Downloads'
end

-- only organize if directory exists
if exists(selectedDir) then
    print('---> organizing files in: ' ..selectedDir)

    -- change directory
    chdir(selectedDir)

    -- create the needed directories
    for keys in pairs(extensions) do
        mkdir(keys)
    end

    -- move files into their respective folders
    function moveFiles(fileGroup, fileTable)
        for _, fileExtension in pairs(fileTable) do
            local matchedFiles = glob('*' ..fileExtension)

            for _, matchedFile in pairs(matchedFiles) do
                move(matchedFile, current() .. '/' .. fileGroup .. '/' .. matchedFile)
            end
        end
    end

    for key in pairs(extensions) do
        moveFiles(key, extensions[key])
    end
else
    print('---> directory doesn\'t exist')
end
