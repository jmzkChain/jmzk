import json

from twisted.internet import reactor

from payengine import PayEngine, prepare_for_debug
from watchpool import WatchPool


class Handler:
    def __call__(self, socket):
        data = socket.recv_string()
        cmd = json.loads(data)
        func = cmd['func']
        if func == 'stop':
            reactor.stop()
        elif func == 'run':
            freq = cmd['freq']
            url = cmd['url']
            users = cmd['users']
            amount = cmd['amount']
            if 'debug' in cmd:
                prepare_for_debug(users)
                symbol = '5,S#666'
            else:
                symbol = '5,S#1'
            pe = PayEngine(freq, users, sym=symbol, amount=amount)
            pe.set_url(url)
            pe.fetch_balances()
            pe.run()
            WatchPool().run()
        elif func == 'watches':
            nodes = cmd['nodes']
            WatchPool().set_nodes(nodes)
            WatchPool().set_socket(socket)
            socket.send_string('watches config succeed')
        else:
            raise Exception('No Such Function')
