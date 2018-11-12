import json
import time

import zmq

ctx = zmq.Context()
socket = ctx.socket(zmq.REQ)
socket.connect('tcp://localhost:6666')

d = {}
d['func'] = 'send'
d['url'] = 'https://mainnet10.everitoken.io/v1/evt_link/get_trx_id_for_link_id'
d['data'] = '{"link_id": "16951b9b653947955faa6c3cb3e506b6"}'
for _ in range(3):
    socket.send_string(json.dumps(d))
    print(socket.recv_string())

d['func'] = 'kill'
socket.send_string(json.dumps(d))

socket.close()
ctx.term()
