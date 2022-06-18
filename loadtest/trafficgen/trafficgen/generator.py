import json
import multiprocessing as mp
import os
import random
import string
from collections import OrderedDict

import click
import tqdm

from pyjmzksdk.action import ActionGenerator
from pyjmzksdk.transaction import TrxGenerator

from . import randompool, utils


class InvalidActionsOrder(Exception):
    def __init__(self, message):
        self.message = message


class GeneratorConfig:
    def __init__(self):
        self.total = 0
        self.max_user_number = 0
        self.actions = OrderedDict()
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

        self.trxgen = TrxGenerator(url, payer=None)
        self.actgen = ActionGenerator()

        self.limits = None
        self.currs = None
        self.total = 0

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


def worker(q, i, url, shuffle, config, output_folder):
    while True:
        name = q.get()
        if name is None:
            break

        gen = TrafficGenerator(name=name, url=url, config=config,
                               output=os.path.join(output_folder, name + '_traffic_data.lz4'))
        gen.initialize()

        pbar = tqdm.tqdm(total=gen.total, desc=name, unit='trx', position=i, ascii=True, leave=False)
        gen.generate(shuffle, lambda x: pbar.update(x))
        pbar.close()

        q.task_done()


@click.command(context_settings=dict(
    ignore_unknown_options=True,
))
@click.option('--url', '-u', default='http://127.0.0.1:8888')
@click.option('--region-num', '-r', default=1)
@click.option('--thread-num', '-j', default=1)
@click.option('--total', '-n', default=10, help='Total number of transactions each region will generate')
@click.option('--max-user-number', '-m', default=10)
@click.option('--shuffle', '-s', is_flag=True)
@click.option('--output-folder', '-o', type=click.Path(exists=False), default='./')
@click.argument('actions', nargs=-1)
def generate(url, region_num, thread_num, total, max_user_number, shuffle, output_folder, actions):
    gen_config = GeneratorConfig()
    gen_config.set_args('total', total)
    gen_config.set_args('max_user_number', max_user_number)

    if len(actions) == 0:
        raise Exception('No actions configured')

    acttypes = ['newdomain', 'issuetoken', 'transfer', 'newfungible', 'issuefungible', 'transferft', 'addmeta']

    for actopt in actions:
        if not actopt.startswith('--act-'):
            raise Exception('Invalid action option')

        acttype, weight = actopt[6:].split('=')
        if acttype not in acttypes:
            raise Exception('Invalid action type')

        gen_config.set_action(acttype, int(weight))

    if sum(gen_config.actions.values()) == 0:
        raise Exception("")

    if not os.path.exists(output_folder):
        os.mkdir(output_folder)

    print('Generating {} regions with config:'.format(region_num), gen_config.dict())

    q = mp.JoinableQueue()
    threads = []

    for i in range(thread_num):
        t = mp.Process(target=worker, args=(q, i, url, shuffle, gen_config, output_folder))
        t.start()
        threads.append(t)

    regions = set()
    for i in range(region_num):
        while True:
            re = ''.join(random.choice(string.ascii_letters[26:]) for __ in range(2))
            if re in regions:
                continue
            regions.add(re)
            break
        q.put(re)
    
    q.join()

    for i in range(thread_num):
        q.put(None)

    for t in threads:
        t.join()


if __name__ == '__main__':
    generate()
