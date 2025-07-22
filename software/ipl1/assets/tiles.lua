local process = require("wf.api.v1.process")
local zx0 = require("wf.api.v1.process.tools.zx0")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

local function gen_palette_mono(values)
        local pal = values[1] | (values[2] << 4) | (values[3] << 8) | (values[4] << 12)
        return process.to_data(string.char(pal & 0xFF, pal >> 8))
end

local tileset = superfamiconv.tiles(
	"tiles.png", gen_palette_mono({0, 7, 2, 5}),
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
)

process.emit_symbol("gfx_tiles", zx0.compress(tileset))
