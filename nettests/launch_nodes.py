import json
import os
import sys
import time

import click
import docker
from pyjmzk import ecc, libjmzk
from pyjmzksdk import action, api, base, transaction


class command():
    """docstring for command."""

    def __init__(self, cmd):
        self.content = cmd

    def add_option(self, option):
        self.content += ' '
        self.content += option

    def get_arguments(self):
        return self.content


# free the container
def free_container(name, client):
    i = -1
    while(True):
        try:
            i += 1
            container = client.containers.get(name+str(i))
            container.stop()
            container.remove()
            click.echo('free {}{} succeed'.format(name, i))
        except docker.errors.NotFound:
            if(i >= 10):
                break

    try:
        container = client.containers.get('postgre')
        container.stop()
        container.remove()
    except docker.errors.NotFound:
        pass
        
    client.volumes.prune()

# free the dir
def free_the_dir(dir):
    list = os.listdir(dir)
    click.echo('remove the files in {}'.format(dir))
    for name in list:
        click.echo(name)
        os.removedirs(os.path.join(dir, name))
    if(not os.path.exists(dir)):
        os.mkdir(dir, 0o755)


@click.group()
def run():
    pass

@click.command()
@click.option('--config', help='the config of nodes', default='launch.config')
def create(config):
    f = open('launch.config', 'r')
    text = f.read()
    f.close()
    paras = json.loads(text)
    producer_number = paras['producer_number']    # the number of the producer
    nodes_number = paras['nodes_number']          # the number of nodes we run
    jmzkd_port_http = paras['jmzkd_port_http']      # the begin port of nodes port,port+1 ....
    jmzkd_port_p2p = paras['jmzkd_port_p2p']        # the begin port of nodes port,port+1 ....
    jmzkd_dir = paras['jmzkd_dir']                  # the data directory of the jmzkd
    use_tmpfs = paras['use_tmpfs']                # use the tmpfs or not
    tmpfs_size = paras['tmpfs_size']              # the memory usage per node
    client = docker.from_env()
    click.echo('check and free the container before')
    free_container('jmzkd_', client)

    try:
        net = client.networks.get("jmzk-net")
    except docker.errors.NotFound:
        network=client.networks.create("jmzk-net",driver="bridge")

    container = client.containers.run(image='bitnami/postgresql:11.1.0',
                                              name='postgre',
                                              network='jmzk-net',
                                              detach=True,
                                              volumes={
                                                  'pg-data-volume': {'bind': '/bitnami', 'mode': 'rw'}}
                                              )

    # begin the nodes one by one
    click.echo('begin open the jmzkd')
    for i in range(0, nodes_number):

        # create files in jmzkd_dir
        if(not os.path.exists(jmzkd_dir)):
            os.mkdir(jmzkd_dir, 0o755)
        file = os.path.join(jmzkd_dir, 'dir_{}'.format(i))
        if(os.path.exists(file)):
            click.echo("Warning: the file before didn't freed ")
        else:
            os.mkdir(file, 0o755)

        # make the command
        cmd = command('jmzkd.sh')
        cmd.add_option('--delete-all-blocks')
        cmd.add_option('--http-validate-host=false')
        cmd.add_option('--charge-free-mode')
        # cmd.add_option('--plugin=jmzk::postgres_plugin')
        # cmd.add_option('--plugin=jmzk::history_plugin')
        # cmd.add_option('--plugin=jmzk::history_api_plugin')
        cmd.add_option('--plugin=jmzk::jmzk_link_plugin')
        cmd.add_option('--plugin=jmzk::chain_api_plugin')
        cmd.add_option('--plugin=jmzk::jmzk_api_plugin')
        # cmd.add_option('--postgres-uri=postgresql://postgres@localhost:5432/jmzk{}'.format(i))

        if(i < producer_number):
            cmd.add_option('--enable-stale-production')
            if (i == 0):
                cmd.add_option('--producer-name=jmzk')
            else:
                cmd.add_option('--producer-name=jmzk{}'.format(i))
            cmd.add_option('--signature-provider=jmzk7vuvMYQwm6WYLoopw6DqhBumM4hC7RA5ufK8WSqU7VQyfmoLwA=KEY:5KZ2HeogGk12U2WwU7djVrfcSami4BRtMyNYA7frfcAnhyAGzKM')

        cmd.add_option('--http-server-address=jmzkd_{}:{}'.format(i, 8888+i))
        cmd.add_option('--p2p-listen-endpoint=jmzkd_{}:{}'.format(i, 9876+i))
        for j in range(0, nodes_number):
            if(i == j):
                continue
            cmd.add_option(('--p2p-peer-address=jmzkd_{}:{}'.format(j, 9876+j)))

        # run the image jmzkd in container
        if(not use_tmpfs):
            click.echo('********jmzkd {} **************'.format(i))
            click.echo('name: jmzkd_{}'.format(i))
            click.echo('nework: jmzk-net')
            click.echo('http port: {} /tcp: {}'.format(jmzkd_port_http+i, 8888+i))
            click.echo('p2p port: {} /tcp: {}'.format(jmzkd_port_p2p+i, 9876+i))
            click.echo('mount location: {}'.format(file))
            click.echo('****************************')
            container = client.containers.run(image='jmzkChain/jmzk:latest',
                                              name='jmzkd_{}'.format(i),
                                              command=cmd.get_arguments(),
                                              network='jmzk-net',
                                              ports={
                                                  '{}'.format(jmzkd_port_http+i): 8888+i, '{}/tcp'.format(jmzkd_port_p2p+i): 9876+i},
                                              detach=True,
                                              volumes={
                                                  file: {'bind': '/opt/jmzkd/data', 'mode': 'rw'}}
                                              )
        else:
            click.echo('********jmzkd {} **************'.format(i))
            click.echo('name: jmzkd_{}'.format(i))
            click.echo('nework: jmzk-net')
            click.echo('http port: {} /tcp: {}'.format(jmzkd_port_http+i, 8888+i))
            click.echo('p2p port: {} /tcp: {}'.format(jmzkd_port_p2p+i, 9876+i))
            click.echo('tmpfs use size: {} M'.format(tmpfs_size))
            click.echo('****************************')
            container = client.containers.run(image='jmzkChain/jmzk:latest',
                                              name='jmzkd_{}'.format(i),
                                              command=cmd.get_arguments(),
                                              network='jmzk-net',
                                              ports={
                                                  '{}'.format(jmzkd_port_http+i): 8888+i, '{}/tcp'.format(jmzkd_port_p2p+i): 9876+i},
                                              detach=True,
                                              tmpfs={
                                                  '/opt/jmzkd/data': 'size='+str(tmpfs_size)+'M'}
                                              #
                                              )
    # update producers
    # producers_json = json.dumps(paras['producers_config'])
    url = 'http://127.0.0.1:8888'
    priv_jmzk = ecc.PrivateKey.from_string(
            '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3')
    pub_jmzk = ecc.PublicKey.from_string(
            'jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV')

    TG = transaction.TrxGenerator(url=url, payer=pub_jmzk.to_string())
    Api = api.Api(url)
    AG = action.ActionGenerator()

    producers_json = {}
    producers = []
    for i in range(0, producer_number):
        dic = {}
        if(i > 0) :
            dic['producer_name'] = "jmzk{}".format(i)
        else :
            dic['producer_name'] = "jmzk"

        dic['block_signing_key'] = "jmzk7vuvMYQwm6WYLoopw6DqhBumM4hC7RA5ufK8WSqU7VQyfmoLwA"
        producers.append(dic)

    producers_json['producers'] = producers
    updsched = AG.new_action_from_json('updsched', json.dumps(producers_json))

    trx = TG.new_trx()
    trx.add_action(updsched)
    trx.add_sign(priv_jmzk)
    Api.push_transaction(trx.dumps())

# format with the click
@click.command()
@click.option('--config', help='the config of nodes', default='launch.config')
def free(config):
    f = open('launch.config', 'r')
    text = f.read()
    f.close()
    paras = json.loads(text)
    free_dir = paras['free_dir']  # delete the directory of the jmzkd
    jmzkd_dir = paras['jmzkd_dir']  # the data directory of the jmzkd
    client = docker.from_env()
    click.echo('free the container')
    free_container('jmzkd_', client)
    if(free_dir):
        click.echo(jmzkd_dir)
        free_the_dir(jmzkd_dir)


if __name__ == '__main__':
    run.add_command(create)
    run.add_command(free)
    run()
