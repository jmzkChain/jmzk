Reader = require("reader")

local counter = 1
local threads = {}
local header = {}
header["Content-Type"] = "application/json"

setup = function(thread)
	thread:set("id", counter)
	table.insert(threads, thread)
	counter = counter + 1
end

init = function(args)
	local regions = args
	trx_num = 0
	resp = 0
	reader = Reader.new('trxs/'..regions[id]..'_traffic_data.lz4')
	print(("thread %d region %s"):format(id, regions[id]))
end

request = function()
	trx = reader:read_trx()
	trx_num = trx_num + 1
	post = wrk.format("POST", "/v1/chain/push_transaction", header, trx)
	return post
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

