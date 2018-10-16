import json
import os
import sys
import time

import click
import docker


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
            print('free {}{} succeed'.format(name, i))
        except docker.errors.NotFound:
            if(i >= 10):
                break

# free the dir


def free_the_dir(dir):
    list = os.listdir(dir)
    print('remove the files in {}'.format(dir))
    for name in list:
        print(name)
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
    evtd_port_http = paras['evtd_port_http']      # the begin port of nodes port,port+1 ....
    evtd_port_p2p = paras['evtd_port_p2p']        # the begin port of nodes port,port+1 ....
    evtd_dir = paras['evtd_dir']                  # the data directory of the evtd
    use_tmpfs = paras['use_tmpfs']                # use the tmpfs or not
    tmpfs_size = paras['tmpfs_size']              # the memory usage per node
    client = docker.from_env()
    print('check and free the container before')
    free_container('evtd_', client)
    # network=client.networks.create("evt-net",driver="bridge")

    # begin the nodes one by one
    print('begin open the evtd')
    for i in range(0, nodes_number):

        # create files in evtd_dir
        if(not os.path.exists(evtd_dir)):
            os.mkdir(evtd_dir, 0o755)
        file = os.path.join(evtd_dir, 'dir_'+str(i))
        if(os.path.exists(file)):
            print("Warning: the file before didn't freed ")
        else:
            os.mkdir(file, 0o755)

        # make the command
        cmd = command('evtd.sh')
        cmd.add_option('--delete-all-blocks')
        if(i < producer_number):
            cmd.add_option('--enable-stale-production')
            cmd.add_option('--producer-name=evt')
        else:
            cmd.add_option('--producer-name=evt.'+str(i))

        cmd.add_option('--http-server-address=evtd_'+str(i)+':'+str(8888+i))
        cmd.add_option('--p2p-listen-endpoint=evtd_'+str(i)+':'+str(9876+i))
        for j in range(0, nodes_number):
            if(i == j):
                continue
            cmd.add_option(('--p2p-peer-address=evtd_'+str(j)+':'+str(9876+j)))

        # run the image evtd in container
        if(not use_tmpfs):
            print('********evtd {} **************'.format(i))
            print('name: evtd_{}'.format(i))
            print('nework: evt-net')
            print('http port: {} /tcp: {}'.format(evtd_port_http+i, 8888+i))
            print('p2p port: {} /tcp: {}'.format(evtd_port_p2p+i, 9876+i))
            print('mount location: {}'.format(file))
            print('****************************')
            container = client.containers.run(image='everitoken/evt:latest',
                                              name='evtd_'+str(i),
                                              command=cmd.get_arguments(),
                                              network='evt-net',
                                              ports={
                                                  str(evtd_port_http+i): 8888+i, str(evtd_port_p2p+i)+'/tcp': 9876+i},
                                              detach=True,
                                              volumes={
                                                  file: {'bind': '/opt/evtd/data', 'mode': 'rw'}}
                                              )
        else:
            print('********evtd {} **************'.format(i))
            print('name: evtd_{}'.format(i))
            print('nework: evt-net')
            print('http port: {} /tcp: {}'.format(evtd_port_http+i, 8888+i))
            print('p2p port: {} /tcp: {}'.format(evtd_port_p2p+i, 9876+i))
            print('tmpfs use size: {} M'.format(tmpfs_size))
            print('****************************')
            container = client.containers.run(image='everitoken/evt:latest',
                                              name='evtd_'+str(i),
                                              command=cmd.get_arguments(),
                                              network='evt-net',
                                              ports={
                                                  str(evtd_port_http+i): 8888+i, str(evtd_port_p2p+i)+'/tcp': 9876+i},
                                              detach=True,
                                              tmpfs={
                                                  '/opt/evtd/data': 'size='+str(tmpfs_size)+'M'}
                                              #
                                              )


# format with the click
@click.command()
@click.option('--config', help='the config of nodes', default='launch.config')
def free(config):
    f = open('launch.config', 'r')
    text = f.read()
    f.close()
    paras = json.loads(text)
    free_dir = paras['free_dir']  # delete the directory of the evtd
    evtd_dir = paras['evtd_dir']  # the data directory of the evtd
    client = docker.from_env()
    print('free the container')
    free_container('evtd_', client)
    if(free_dir):
        print(evtd_dir)
        free_the_dir(evtd_dir)


if __name__ == '__main__':
    run.add_command(create)
    run.add_command(free)
    run()
