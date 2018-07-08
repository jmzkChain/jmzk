import json
import random
from collections import OrderedDict

from pyevtsdk.action import ActionGenerator
from pyevtsdk.transaction import TrxGenerator

import utils, randompool


class InvalidActionsOrder(Exception):
    def __init__(self, message):
        self.message = message


class TrafficGenerator:
    def __init__(self, name, url, config='actions.config', output='traffic_data.lz4'):
        self.conf = self.load_conf(config)
        self.name = name

        self.rp = randompool.RandomPool(
            tg_name=self.name, max_user_num=self.conf['max_user_number'])
        self.writer = utils.Writer(output)

        self.trxgen = TrxGenerator(url)
        self.actgen = ActionGenerator()

        self.limits, self.currs, self.total = self.init_actions()

    @staticmethod
    def load_conf(config):
        with open(config, 'r') as f:
            conf = json.load(f, object_pairs_hook=OrderedDict)
        return conf

    def init_actions(self):
        limits = {}
        currs = {}

        total = self.conf['total']
        actions = self.conf['actions']
        ratio_sum = sum(actions.values())
        for action, ratio in actions.items():
            if ratio == 0:
                continue
            limits[action] = ratio / ratio_sum * total
            currs[action] = 0

        total = sum(limits.values())
        return limits, currs, total

    def generate(self, shuffle=True, process_cb=None):
        actions = list(self.limits.keys())

        i = 0
        while len(actions) > 0:
            if shuffle:
                act = random.choice(actions)
                if not self.rp.satisfy_action(act):
                    continue
            else:
                act = actions[0]
                if not self.rp.satisfy_action(act):
                    raise InvalidActionsOrder("{} action is not satisfied in current order, try to adjust".format(act))

            self.currs[act] += 1
            if self.currs[act] >= self.limits[act]:
                actions.remove(act)

            args, priv_keys = self.rp.require(act)
            action = self.actgen.new_action(act, **args)
            
            trx = self.trxgen.new_trx()
            trx.add_action(action)
            for priv_key in priv_keys:
                trx.add_sign(priv_key)

            self.writer.write_trx(trx.dumps())

            i = i + 1
            if process_cb is not None:
                process_cb(i / self.total)

        self.writer.close()


if __name__ == '__main__':
    TG = TrafficGenerator(name='TESTA', url="http://127.0.0.1:8888")
    TG.generate()
