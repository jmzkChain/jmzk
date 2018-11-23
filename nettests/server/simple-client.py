import json
import time

import zmq

ctx = zmq.Context()
socket = ctx.socket(zmq.REQ)
socket.connect('tcp://localhost:6666')


def watches_config():
    d = {}
    d['func'] = 'watches'
    d['nodes'] = ['http://127.0.0.1:8888']
    return d


def everipay():
    d = {}
    d['func'] = 'run'
    d['url'] = 'http://127.0.0.1:8888'
    d['freq'] = 2
    d['users'] = '/home/fieryzig/payers.json'
    d['amount'] = 5
    d['debug'] = 1
    return d


if __name__ == '__main__':
    socket = ctx.socket(zmq.REQ)
    socket.connect('tcp://localhost:6666')

    j = json.dumps(watches_config())
    socket.send_string(j)
    print(socket.recv_string())

    j = json.dumps(everipay())
    socket.send_string(j)
    print(socket.recv_string())

    socket = ctx.socket(zmq.REQ)
    socket.connect('tcp://localhost:6666')
    
    j = json.dumps(watches_config())
    socket.send_string(j)
    print(socket.recv_string())

    j = json.dumps(everipay())
    socket.send_string(j)
    print(socket.recv_string())

    socket.close()
    ctx.term()
