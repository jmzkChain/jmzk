local ffi = require("ffi")
local lz4 = ffi.load("lz4")
ffi.cdef[[
int LZ4_compress(const char* source, char* dest, int isize);
int LZ4_uncompress_unknownOutputSize(const char * source, char* dest, int isize, int max_size);
int LZ4_compressBound(int isize);
]]
local compress = function(src)
	local dest_len = lz4.LZ4_compressBound(#src)
	local dest = ffi.new("uint8_t[?]", dest_len)
	local len = lz4.LZ4_compress(src, dest, #src)
	if len == 0 then
		return nil
	else
		return ffi.string(dest, len)
	end
end
local uncompress = function(src, osize)
	local dest = ffi.new("uint8_t[?]", osize)
	local len = lz4.LZ4_uncompress_unknownOutputSize(src, dest, #src, osize)
	if (len > 0) then
		return ffi.string(dest, len)
	else
		return nil
	end
end

local _class = {}
function class(super)
	local class_type={}
	class_type.ctor=false
	class_type.super=super
	class_type.new=function(...) 
			local obj={}
			do
				local create
				create = function(c,...)
					if c.super then
						create(c.super,...)
					end
					if c.ctor then
						c.ctor(obj,...)
					end
				end
 
				create(class_type,...)
			end
			setmetatable(obj,{ __index=_class[class_type] })
			return obj
		end
	local vtbl={}
	_class[class_type]=vtbl
 
	setmetatable(class_type,{__newindex=
		function(t,k,v)
			vtbl[k]=v
		end
	})
 
	if super then
		setmetatable(vtbl,{__index=
			function(t,k)
				local ret=_class[super][k]
				vtbl[k]=ret
				return ret
			end
		})
	end
 
	return class_type
end

local Reader = class()
function Reader:ctor(region)
    self.region = region
    self.file = io.open(region, 'rb')
    if self.file==nil then
        print('Error open file '..self.region)
    end
end

function Reader:read_trx()
    header = self.file:read(2)
    if header == nil then return nil end
    r_size = string.byte(header,1) + string.byte(header,2)*256
    header = self.file:read(2)
    if header == nil then return nil end
    c_size = string.byte(header,1) + string.byte(header,2)*256
    lz4_trx = self.file:read(c_size)
    if lz4_trx == nil then
        print('Error read trx')
        return nil
    end
    return uncompress(lz4_trx, r_size)
end

return Reader
