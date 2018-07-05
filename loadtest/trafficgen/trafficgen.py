import json
import random
from collections import deque

from pyevtsdk.action import ActionGenerator
from pyevtsdk.transaction import TrxGenerator

from . import randompool, utils

AG = ActionGenerator()
Trx = TrxGenerator('http://118.31.58.10:8888')


class TrafficGen:
    def __init__(self, name, config='actions.config', output='traffic_data.lz4'):
        self.conf = self.handle_conf(config)
        self.name = name
        self.rp = randompool.RandomPool(
            tg_name=self.name, max_user_num=self.conf['max_user_number'])
        self.action_queue = self.make_queue(
            self.conf['total'], self.conf['actions'])
        self.writer = utils.Writer(self.name+'_'+output)

    def handle_conf(self, config):
        with open(config, 'r') as f:
            conf = json.load(f)
        return conf

    def make_queue(self, total, actions):
        ratio_sum = sum(actions.values())
        q = deque()
        for action, ratio in actions.items():
            q.extend([action] * round(total * ratio/ratio_sum))
        random.shuffle(q)
        return q

    def generator(self):
        while self.action_queue:
            action_type = self.action_queue.popleft()
            if not self.rp.satisfy_action(action_type):
                self.action_queue.append(action_type)
                continue
            args, priv_keys = self.rp.require(action_type)
            action = AG.new_action(action_type, **args)
            trx = Trx.new_trx()
            trx.add_action(action)
            [trx.add_sign(priv_key) for priv_key in priv_keys]

            self.writer.write_trx(trx.dumps())

        self.writer.close()


if __name__ == '__main__':
    TG = TrafficGen(name='test')
    TG.generator()
