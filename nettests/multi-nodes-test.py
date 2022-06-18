import subprocess
import unittest
import docker
import click
import time
import json
import zmq

from pyjmzk import abi, ecc, jmzk_link, libjmzk
from pyjmzksdk import action, api, base, transaction
import requests

import launch_nodes
ctx = zmq.Context()
socket = ctx.socket(zmq.REQ)
socket.connect('tcp://localhost:6666')

def watches_config():
    d = {}
    d['func'] = 'watches'
    d['nodes'] = ['http://127.0.0.1:8888','http://127.0.0.1:8889','http://127.0.0.1:8890','http://127.0.0.1:8891','http://127.0.0.1:8893']
    return d


def everipay():
    d = {}
    d['func'] = 'run'
    d['url'] = 'http://127.0.0.1:8888'
    d['freq'] = 2
    d['users'] = 'payers.json'
    d['amount'] = 10
    d['debug'] = 1
    return d

class Test(unittest.TestCase):
    def _test_block(self, dif):
        i = -1
        Min = 99999999999999
        Max = -1
        while(i<4):
            i+=1
            tmp = requests.get('http://127.0.0.1:{}/v1/chain/get_info'.format(8888+ i)).json()['head_block_num']
            Min = min(Min, tmp)
            Max = max(Max, tmp)
        self.assertTrue(Max-Min<dif, msg = '{} {}'.format(Min, Max))

        for i in range(5):
            socket = ctx.socket(zmq.REQ)
            socket.connect('tcp://localhost:6666')
            j = json.dumps(everipay())
            socket.send_string(j)
            rep = socket.recv_string()
            self.assertTrue(rep == 'Success', msg =rep)

    def test_1(self):
        p = subprocess.Popen(['pumba', 'netem', '--tc-image', 'gaiadocker/iproute2', '--duration', '5m', 'delay', '--time', '1000', '--jitter', '500', 'jmzkd_0'],
                             stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=False)
        time.sleep(2)
        self._test_block(20)
        print('kill')
        p.kill()

    def test_2(self):
        p = subprocess.Popen(['pumba', 'netem', '--tc-image', 'gaiadocker/iproute2', '--duration', '5m', 'loss', '--percent', '20', 'jmzkd_0'],
                             stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=False)
        time.sleep(2)
        self._test_block(20)
        print('kill')
        p.kill()

    def test_3(self):
        subps = []
        cmd = ['pumba', 'netem', '--tc-image', 'gaiadocker/iproute2', '--duration', '5m', 'delay', '--time', '3000', '--jitter', '500']
        for i in range(5):
            cmd_t = cmd
            cmd_t.append('jmzkd_{}'.format(i))
            p = subprocess.Popen(cmd_t, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=False)
            subps.append(p)
        time.sleep(2)
        self._test_block(100)
        print('kill')
        for p in subps:
            p.kill()

@click.command()
def main():
    j = json.dumps(watches_config())
    socket.send_string(j)
    print(socket.recv_string())

    global api
    api = api.Api('http://127.0.0.1:8888')

    suite = unittest.TestLoader().loadTestsFromTestCase(Test)
    runner = unittest.TextTestRunner()
    result = runner.run(suite)

if __name__ == '__main__':
    main()