-- add cgame require path
package.path = package.path .. ';' .. cgame_data_path .. '/script/?.lua'

-- load 'cgame' module -- it makes itself available globally
require 'cgame'

-- some aliases
cg = cgame
cs = cgame.systems
cg.args = cgame_args

-- print to console
print = function (k) cs.console.puts(tostring(k)) end

-- got startup script argument?
if cg.args[1] then dofile(cg.args[1]) end
