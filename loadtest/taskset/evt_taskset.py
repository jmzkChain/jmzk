import os
import random
import shutil
import string
import sys

from locust import TaskSet, task
from locust.contrib.fasthttp import FastHttpLocust
from trafficgen.generator import TrafficGenerator
from trafficgen.utils import Reader

hosturl = 'http://127.0.0.1:8888'

if len(sys.argv) > 0:
    hosturl = sys.argv[1]


def generate_traffic():
    nonce = ''.join(random.choice(string.ascii_letters[26:]) for _ in range(5))

    path = '/tmp/evt_loadtest'
    if os.path.exists(path):
        shutil.rmtree(path)
    os.mkdir(path)

    traffic = '{}/{}.lz4'.format(path, nonce)
    gen = TrafficGenerator(nonce, hosturl, 'actions.config', traffic)

    print('{} Generating traffic'.format(nonce))
    gen.generator()
    print('{} Generating traffic - OK'.format(nonce))

    return Reader(traffic), nonce


class EVTTaskSet(TaskSet):
    def __init__(self, parent):
        super().__init__(parent)
        self.reader = None
        self.nonce = None
        self.stop = False

    def on_start(self):
        self.reader, self.nonce = generate_traffic()

    @task(1)
    def push_trx(self):
        if self.stop:
            return

        try:
            trx = self.reader.read_trx()
        except:
            print('{} Traffic has been sent completed'.format(self.nonce))
            self.stop = True
            return

        with self.client.post('/v1/chain/push_transaction', trx, catch_response=True) as response:
            if response.status_code == 202:
                response.success()


class WebsiteUser(FastHttpLocust):
    task_set = EVTTaskSet
    host = hosturl
    min_wait = 0
    max_wait = 1000
