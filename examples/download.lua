local apolo = require 'apolo'

version_number = apolo.read('VERSION')
ddg_robots = assert(apolo.read('https://duckduckgo.com/robots.txt'))

print('Version number: ' .. version_number)
print(ddg_robots)
