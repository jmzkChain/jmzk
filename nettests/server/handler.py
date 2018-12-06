import json

from twisted.internet import reactor

from payengine import PayEngine, prepare_for_debug
from watchpool import WatchPool

context = {}

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
            
            context[socket]['WP'].alive = True
            pe = PayEngine(freq, users, watch_pool=context[socket]['WP'], sym=symbol, amount=amount)
            pe.set_url(url)
            pe.fetch_balances()
            pe.run()
            context[socket]['PE'] = pe
            context[socket]['WP'].run()
        elif func == 'watches':
            nodes = cmd['nodes']
            wp = WatchPool()
            wp.set_nodes(nodes)
            wp.set_socket(socket)
            context[socket] = {}
            context[socket]['WP'] = wp
            socket.send_string('watches config succeed')
        else:
            raise Exception('No Such Function')
