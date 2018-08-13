local reader = require("reader")

local threads = {}
local header = {
    "Content-Type" = "application/json"
}

setup = function(thread)
	thread:set("id", #threads)
    thread:set("reader", reader)
	table.insert(threads, thread)
end

init = function(args)
	local regions = select(2, unpack(args))
    local folder = args[1]

	trx_num = 0
	resp = 0
	file = reader.open(folder..regions[id]..'_traffic_data.lz4')
	print(("thread %d region %s"):format(id, regions[id]))
end

request = function()
	trx = reader.read_trx(file)
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

