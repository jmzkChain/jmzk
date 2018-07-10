import json
import multiprocessing as mp
import os
import random
import string
from collections import OrderedDict

import click
import tqdm

from pyevtsdk.action import ActionGenerator
from pyevtsdk.transaction import TrxGenerator

from . import randompool, utils


class InvalidActionsOrder(Exception):
    def __init__(self, message):
        self.message = message


class GeneratorConfig:
    def __init__(self):
        self.total = 0
        self.max_user_number = 0
        self.actions = {}
        self.output = ''

    def set_args(self, attr, value):
        self.__setattr__(attr, value)

    def set_action(self, action, value):
        self.actions[action] = value

    def dict(self):
        return {'total': self.total, 'max_user_number': self.max_user_number, 'actions': self.actions}


class TrafficGenerator:
    def __init__(self, name, url, config, output='traffic_data.lz4'):
        assert len(name) <= 2, 'Length of region name should be less than 2'

        self.conf = config.dict()
        self.name = name

        self.rp = None
        self.writer = utils.Writer(output)

        self.trxgen = TrxGenerator(url)
        self.actgen = ActionGenerator()

        self.limits = None
        self.currs = None
        self.total = 0

    @staticmethod
    def load_conf(config):
        with open(config, 'r') as f:
            conf = json.load(f, object_pairs_hook=OrderedDict)
        return conf

    def initialize(self):
        self.rp = randompool.RandomPool(
            tg_name=self.name, max_user_num=self.conf['max_user_number'])
        self.init_actions()

    def init_actions(self):
        limits = {}
        currs = {}

        total = self.conf['total']
        actions = self.conf['actions']
        ratio_sum = sum(actions.values())
        for action, ratio in actions.items():
            if ratio == 0:
                continue
            limits[action] = round(ratio / ratio_sum * total)
            currs[action] = 0

        total = sum(limits.values())
        self.limits = limits
        self.currs = currs
        self.total = total

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
                    raise InvalidActionsOrder(
                        '{} action is not satisfied in current order, try to adjust'.format(act))

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
                process_cb(1)

        self.writer.close()


def worker(q, id, url, config, output_folder):
    while True:
        name = q.get()
        if name == None:
            break
        gen = TrafficGenerator(name=name, url=url, config=config, output=os.path.join(output_folder, name+'_traffic_data.lz4'))
        gen.initialize()

        with tqdm.tqdm(total=gen.total) as pbar:
            gen.generate(True, lambda x: pbar.update(x))

        q.task_done()


@click.command()
@click.option('--url', default='http://127.0.0.1:8888')
@click.option('--region-num', default=1)
@click.option('--thread-num', default=1)
@click.option('--total', default=10)
@click.option('--max-user-number', default=10)
@click.option('--action-newdomain', default=0)
@click.option('--action-issuetoken', default=0)
@click.option('--action-transfer', default=0)
@click.option('--action-newfungible', default=0)
@click.option('--action-issuefungible', default=0)
@click.option('--action-transferft', default=0)
@click.option('--action-addmeta', default=0)
@click.option('--output-folder', default='./')
def main(url, region_num, thread_num, total, max_user_number, action_newdomain, action_issuetoken, action_transfer, action_newfungible, action_issuefungible, action_transferft, action_addmeta, output_folder):
    gen_config = GeneratorConfig()
    gen_config.set_args('total', total // region_num)
    gen_config.set_args('max_user_number', max_user_number)
    gen_config.set_action('newdomain', action_newdomain)
    gen_config.set_action('issuetoken', action_issuetoken)
    gen_config.set_action('transfer', action_transfer)
    gen_config.set_action('newfungible', action_newfungible)
    gen_config.set_action('issuefungible', action_issuefungible)
    gen_config.set_action('transferft', action_transferft)
    gen_config.set_action('addmeta', action_addmeta)

    if not os.path.exists(output_folder):
        os.mkdir(output_folder)

    q = mp.JoinableQueue()
    threads = []
    
    for i in range(thread_num):
        t = mp.Process(target=worker, args=(q, i, url, gen_config, output_folder))
        t.start()
        threads.append(t)

    for _ in range(region_num):
        region_name = ''.join(random.choice(
            string.ascii_letters[26:]) for __ in range(2))
        q.put(region_name)
    q.join()


    for i in range(region_num):
        q.put(None)

    for t in threads:
        t.join()


if __name__ == '__main__':
    main()
