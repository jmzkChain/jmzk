#!/usr/bin/env python3

import os
import sys
import time

import click
import docker


def remove_dir(dir):
    items = os.listdir(dir)
    for item in items:
        os.remove(os.path.join(dir, item))


def get_info_in_content(content, writer):
    head = '"reqs","fails","Avg","Min","Max","Median","req/s"'+'\n'
    writer.write(head)
    lines = content.splitlines()
    for line in lines:
        if('POST' in line):
            strs = line.split(' ')
            strs = [i for i in strs if i is not '']
            if(len(strs) == 10):
                l = ''
                for i in range(2, 10):
                    if(i == 7):
                        continue
                    l += (str(strs[i])+',')
                writer.write(l[:-1]+'\n')
            if(len(strs) == 12):
                head = '"reqs","50%","66%","75%","80%","90%","95%","98%","99%","100%"'+'\n'
                writer.write(head)
                l = ''
                for i in range(2, 12):
                    l += (str(strs[i])+',')

                writer.write(l[:-1]+'\n')


def run_the_traffic(index, host, max_user, regin_number, work_thread, path, param):
    client = docker.from_env()
    command = '-u '+host+' -r ' + \
        str(regin_number)+' -j '+str(work_thread) + \
        ' -m '+str(max_user)+' '+param
    print(command)
    container = client.containers.run(image='jmzkChain/trafficgen',
                                      command=command,
                                      name='trafficgen_'+str(index),
                                      network='jmzk-net',
                                      hostname='127.0.0.1',
                                      detach=False,
                                      volumes={
                                          path: {'bind': '/opt/traffic', 'mode': 'rw'}}
                                      )
    container = client.containers.get('trafficgen_'+str(index))
    print(container.logs(stdout=True))
    container.stop()
    container.remove()


def run_the_loadtest(index, host, log_dir, max_user, increase_rate, run_time, path, slave_number):
    client = docker.from_env()
    for j in range(0, slave_number+1):
        command = ''
        if(j == 0):
            command = ' --master --host='+host+' --no-web -c ' + \
                str(max_user)+' -r '+str(increase_rate)+' -t '+str(run_time)
            container = client.containers.run(image='jmzkChain/loadtest:latest',
                                              command=command,
                                              name='loadtest_' +
                                              str(index)+'_'+str(j),
                                              network='jmzk-net',
                                              hostname='127.0.0.1',
                                              ports={'9999': 9999},
                                              detach=True,
                                              volumes={
                                                  path+'_'+str(j): {'bind': '/opt/traffic', 'mode': 'rw'}}
                                              # environment={"host":host}
                                              )
        else:
            ip = os.popen(
                "docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' loadtest_"+str(index)+'_0').read()
            print(ip)
            print(path)
            command = ' --slave --master-host='+str(ip)+' '
            container = client.containers.run(image='jmzkChain/loadtest:latest',
                                              command=command,
                                              name='loadtest_' +
                                              str(index)+'_'+str(j),
                                              network='jmzk-net',
                                              hostname='127.0.0.1',
                                              # ports={"9999":9999},
                                              detach=True,
                                              volumes={
                                                  path+'_'+str(j-1): {'bind': '/opt/traffic', 'mode': 'rw'}}
                                              # environment={"host":host}
                                              )

    time.sleep(run_time+50)
    # try:
    container = client.containers.get('loadtest_'+str(index)+'_'+str(0))
    log_name = 'loadtest'+str(index)+'.logs'
    f = open(os.path.join(log_dir, log_name), 'w')
    print(container.logs(stdout=True))
    get_info_in_content(container.logs(stdout=True), f)
    container.stop()
    container.remove()
    f.close()

    for j in range(0, slave_number):
        container = client.containers.get('loadtest_'+str(index)+'_'+str(j+1))
        print(container.logs(stdout=True))
        container.stop()
        container.remove()


@click.group()
def run():
    pass


@click.command()
@click.option('--max_user', help='the max number of user', type=click.IntRange(1, 10000000), default=15)
@click.option('--increase_rate', help='the increase_rate of the user', type=click.IntRange(1, 10000000), default=1)
@click.option('--run_time', help='the time locust run', type=click.IntRange(1, 1000000000), default=200)
@click.option('--jmzk_host', help='the host of the string', type=str, default='http://jmzkd_0:8888')
@click.option('--log_dir', help='the dir of the log file', type=click.Path('rw'), default='/home/harry/loadtest_logs')
@click.option('--config_file', help='the config of the loadtest', type=click.Path('rw'), default='/home/harry/loadtest/loadtest.config')
@click.option('--traffic_data_dir', help='the config of the loadtest', type=click.Path('rw'), default='/home/harry/traffic_data/')
@click.option('--work_thread', help='the work thread for the traffic generator', type=click.IntRange(1, 10000000), default=4)
@click.option('--regin_number', help='the work thread for the traffic generator', type=click.IntRange(1, 10000000), default=15)
@click.option('--slave_number', help='the work thread for the traffic generator', type=click.IntRange(1, 10000000), default=4)
def begin(max_user, increase_rate, run_time, jmzk_host, log_dir, config_file, traffic_data_dir, work_thread, regin_number, slave_number):
    i = 0
    f = open(config_file)
    line = f.readline()
    while(line):
        path_prefix = 'traffic_data_'+str(i)
        for j in range(0, slave_number):
            print('begin run the traffic generator index:'+str(i)+','+str(j))
            dir_name = path_prefix+'_'+str(j)
            path = os.path.join(traffic_data_dir, dir_name)
            if(not os.path.exists(path)):
                os.mkdir(path, 0755)
            run_the_traffic(i, jmzk_host, max_user,
                            regin_number, work_thread, path, line)
        print('begin run the loadtest index:'+str(i))
        run_the_loadtest(i, jmzk_host, log_dir, max_user, increase_rate, run_time, os.path.join(
            traffic_data_dir, path_prefix), slave_number)
        items = os.listdir(traffic_data_dir)
        for item in items:
            if(path_prefix in item):
                remove_dir(os.path.join(traffic_data_dir, item))
        i += 1
        line = f.readline()
    f.close()


@click.command()
@click.option('--slave_number', help='the work thread for the traffic generator', type=click.IntRange(1, 10000000), default=1)
def free_container(slave_number):
    i = -1
    client = docker.from_env()
    conti = True
    while(conti):
        i += 1
        for j in range(0, slave_number+1):
            try:
                name = 'loadtest_'
                container = client.containers.get(name+str(i)+'_'+str(j))
                container.stop()
                container.remove()
                print('free '+name+str(i)+'_'+str(j)+' succeed')
            except docker.errors.NotFound:
                if(i >= 10):
                    conti = False
                    break
    i = -1
    while(True):
        try:
            i += 1
            name = 'trafficgen_'
            container = client.containers.get(name+str(i))
            container.stop()
            container.remove()
            print('free '+name+str(i)+' succeed')
        except docker.errors.NotFound:
            if(i >= 10):
                break


@click.command()
@click.option('--log_dir', help='the dir of the log file', type=click.Path('rw'), default='/home/harry/loadtest_logs')
def free_logs(log_dir):
    list = os.listdir(log_dir)
    print('remove the files in '+log_dir)
    for name in list:
        if('.logs' in name):
            print(name)
            os.remove(os.path.join(log_dir, name))
    if(not os.path.exists(log_dir)):
        os.mkdir(log_dir, 0755)


@click.command()
@click.option('--traffic_data_dir', help='the config of the loadtest', type=click.Path('rw'), default='/home/harry/traffic_data/')
def free_traffic(traffic_data_dir):
    list = os.listdir(traffic_data_dir)
    print('remove the files in '+traffic_data_dir)
    for name in list:
        if('traffic_data_' in name):
            print(name)
            path = os.path.join(traffic_data_dir, name)
            items = os.listdir(path)
            for item in items:
                os.remove(os.path.join(path, item))
            os.removedirs(path)
    if(not os.path.exists(traffic_data_dir)):
        os.mkdir(traffic_data_dir, 0755)


if __name__ == '__main__':
    run.add_command(begin)
    run.add_command(free_container)
    run.add_command(free_logs)
    run.add_command(free_traffic)
    run()
