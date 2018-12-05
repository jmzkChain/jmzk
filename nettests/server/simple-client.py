import json
import time

import zmq

ctx = zmq.Context()
socket = ctx.socket(zmq.REQ)
socket.connect('tcp://localhost:6666')


def watches_config():
    d = {}
    d['func'] = 'watches'
    d['nodes'] = ['http://127.0.0.1:8891','http://127.0.0.1:8892','http://127.0.0.1:8893']
    return d


def everipay():
    d = {}
    d['func'] = 'run'
    d['url'] = 'http://127.0.0.1:8891'
    d['freq'] = 2
    d['users'] = 'payers.json'
    d['amount'] = 10
    d['debug'] = 1
    return d


if __name__ == '__main__':
    socket = ctx.socket(zmq.REQ)
    socket.connect('tcp://localhost:6666')

    j = json.dumps(watches_config())
    socket.send_string(j)
    print(socket.recv_string())

    while True:
        socket = ctx.socket(zmq.REQ)
        socket.connect('tcp://localhost:6666')
        j = json.dumps(everipay())
        socket.send_string(j)
        print(socket.recv_string())

    socket.close()
    ctx.term()
