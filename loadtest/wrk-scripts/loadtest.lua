local ffi = require("ffi")
local lz4 = ffi.load("lz4")

ffi.cdef[[
int LZ4_compress(const char* source, char* dest, int isize);
int LZ4_uncompress_unknownOutputSize(const char * source, char* dest, int isize, int max_size);
int LZ4_compressBound(int isize);
]]

local uncompress = function(src, osize)
    local dest = ffi.new("uint8_t[?]", osize)
    local len = lz4.LZ4_uncompress_unknownOutputSize(src, dest, #src, osize)
    if (len > 0) then
        return ffi.string(dest, len)
    else
        return nil
    end
end

local function openregion(region)
    local file = io.open(region, 'rb')
    if file == nil then
        error('error while openning file: ' .. region)
    end
    return file
end

local function readsz(file)
    local b = file:read(2)
    if b == nil then
        return nil
    end
    return string.byte(b, 1) + string.byte(b, 2) * 256
end

local function readtrx(file)
    r_size = readsz(file)
    c_size = readsz(file)

    if r_size == nil or c_size == nil then
        return nil
    end

    lz4_trx = file:read(c_size)
    if lz4_trx == nil then
        error('error while reading trx')
    end
    return uncompress(lz4_trx, r_size)
end

local threads = {}
local header = {
    ["Content-Type"] = "application/json"
}

setup = function(thread)
    thread:set("id", #threads + 1)
	thread:set("header", header)
	table.insert(threads, thread)
end

init = function(args)
	local regions = { select(2, unpack(args)) }
    local folder = args[1]

	trx_num = 0
	resp = 0
	file = openregion(string.format("%s/%s_traffic_data.lz4", folder, regions[id]))
	print(("thread %d region %s"):format(id, regions[id]))
end

request = function()
	trx = readtrx(file)

    if trx == nil then
        wrk.thread:stop()
    end

	trx_num = trx_num + 1
	return wrk.format("POST", "/v1/chain/push_transaction", header, trx)
end

response = function(status, headers, body)
	resp = resp + 1
end

done = function(summary, latency, requests)
	for _, thd in ipairs(threads) do
		local id = thd:get("id")
		local trx_num = thd:get("trx_num")
		local resp = thd:get("resp")
		local msg = "thd %d push %d trxs got %d resps"
		print(msg:format(id, trx_num, resp))
	end
end

