#!/usr/bin/env python3

from datetime import datetime

import click
import docker
import docker.errors

client = docker.from_env()


def green(text):
    return click.style(text, fg='green')


@click.group('cli')
def cli():
    pass


@cli.command()
@click.argument('name')
def start(name):
    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is already running and cannot start'.format(green(name)))
            return

        container.start()
        click.echo('{} container is started'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))


@cli.command()
@click.argument('name')
def stop(name):
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is already stopped'.format(green(name)))
            return

        container.stop()
        click.echo('{} container is stopped'.format(green(name)))
    except docker.errors.NotFound:
        click.echo(
            '{} container is not existed, please start first'.format(green(name)))


@cli.command()
@click.argument('name')
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
def logs(name, tail, stream):
    try:
        container = client.containers.get(name)
        s = container.logs(stdout=True, tail=tail, stream=stream)
        if stream:
            for line in s:
                click.echo(line, nl=False)
        else:
            click.echo(s.decode('utf-8'))
    except docker.errors.NotFound:
        click.echo(
            '{} container is not existed, please start first'.format(green(name)))


@cli.command()
@click.argument('name')
def detail(name):
    client = docker.APIClient()
    containers = [c for c in client.containers(
        all=True, filters={'name': name}) if c['Names'][0] == '/' + name]
    if len(containers) == 0:
        click.echo('{} container is not found'.format(green(name)))
        return

    ct = containers[0]

    ports = []
    for p in ct['Ports']:
        if p['PrivatePort'] == 8888:
            p['Type'] += '(http)'
        if p['PrivatePort'] == 7888:
            p['Type'] += '(p2p)'

        ports.append(
            '{}:{}->{}/{}'.format(p['IP'], p['PublicPort'], p['PrivatePort'], p['Type']))

    volumes = []
    for v in ct['Mounts']:
        volumes.append('{}->{}'.format(v['Name'], v['Destination']))

    click.echo('      id: {}'.format(green(ct['Id'])))
    click.echo('   image: {}'.format(green(ct['Image'])))
    click.echo('image-id: {}'.format(green(ct['ImageID'])))
    click.echo(' command: {}'.format(green(ct['Command'])))
    click.echo(' network: {}'.format(
        green(list(ct['NetworkSettings']['Networks'].keys())[0])))
    click.echo('   ports: {}'.format(green(', '.join(ports))))
    click.echo(' volumes: {}'.format(green(', '.join(volumes))))
    click.echo('  status: {}'.format(green(ct['Status'])))


@cli.group('network')
@click.option('--name', '-n', default='evt-net', help='Name of the network for the environment')
@click.pass_context
def network(ctx, name):
    ctx.ensure_object(dict)
    ctx.obj['name'] = name


@network.command()
@click.pass_context
def init(ctx):
    name = ctx.obj['name']
    try:
        client.networks.get(name)
        click.echo(
            'Network: {} network is already existed, no need to create one'.format(green(name)))
    except docker.errors.NotFound:
        client.networks.create(name)
        click.echo('Network: {} network is created'.format(green(name)))


@network.command()
@click.pass_context
def clean(ctx):
    name = ctx.obj['name']
    try:
        net = client.networks.get(name)
        net.remove()
        click.echo('Network: {} network is deleted'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('Network: {} network is not existed'.format(green(name)))


@cli.group('postgres')
@click.option('--name', '-n', default='pg', help='Name of the postgres container')
@click.pass_context
def postgres(ctx, name):
    ctx.ensure_object(dict)
    ctx.obj['name'] = name


@postgres.command()
@click.pass_context
def init(ctx):
    name = ctx.obj['name']

    try:
        client.images.get('bitnami/postgresql:11.1.0')
        click.echo('{} is already existed, no need to fetch again'.format(
            green('bitnami/postgresql:11.1.10')))
    except docker.errors.ImageNotFound:
        click.echo('Pulling latest postgres image...')
        client.images.pull('bitnami/postgresql', '11.1.0')
        click.echo('Pulled latest postgres image')

    volume_name = '{}-data-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('{} volume is already existed, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('{} volume is created'.format(green(volume_name)))


@postgres.command()
@click.option('--net', '-n', default='evt-net', help='Name of the network for the environment')
@click.option('--port', '-p', default=5432, help='Expose port for postgres')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for postgres')
@click.pass_context
def create(ctx, net, port, host, dbname):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    image = 'bitnami/postgresql:11.1.0'

    try:
        client.images.get(image)
        client.networks.get(net)
        client.volumes.get(volume_name)
    except docker.errors.ImageNotFound:
        click.echo(
            'Some necessary elements are not found, please run `postgres init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `postgres init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is existed but not running, try to remove old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                '{} container is already existed and running, cannot restart, run `postgres stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    client.containers.create(image, None, name=name, detach=True, network=net,
                             ports={'5432/tcp': (host, port)},
                             volumes={volume_name: {
                                 'bind': '/bitnami', 'mode': 'rw'}},
                             )
    click.echo('{} container is created'.format(green(name)))


@postgres.command()
@click.argument('dbname', default='evt')
@click.pass_context
def createdb(ctx, dbname):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    image = 'bitnami/postgresql:11.1.0'

    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is not running, cannot create database'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))
        return

    cmd1 = 'psql -U postgres -c "SELECT EXISTS(SELECT datname FROM pg_catalog.pg_database WHERE datname=\'{}\');"'.format(
        dbname)
    c1, logs1 = container.exec_run(cmd1)

    if 't\n' in logs1.decode('utf-8'):
        click.echo(
            '{} database is already created, skip creation'.format(green(name)))
        return

    cmd2 = "psql -U postgres -c 'CREATE DATABASE {}'".format(dbname)
    c2, logs2 = container.exec_run(cmd2)

    if 'CREATE DATABASE' in logs2.decode('utf-8'):
        click.echo('Created database: {} in postgres'.format(green(dbname)))


@postgres.command()
@click.option('--all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, cannot clean'.format(green(name)))
            return

        container.remove()
        click.echo('{} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove()
        click.echo('{} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('{} volume is not existed'.format(green(volume_name)))


@postgres.command('start')
@click.pass_context
def startpostgres(ctx):
    ctx.invoke(start, name=ctx.obj['name'])


@postgres.command('stop')
@click.pass_context
def stoppostgres(ctx):
    ctx.invoke(stop, name=ctx.obj['name'])


@postgres.command('logs')
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
@click.pass_context
def postgreslogs(ctx, tail, stream):
    ctx.forward(logs, name=ctx.obj['name'])


@postgres.command('detail')
@click.pass_context
def detailpostgres(ctx):
    ctx.invoke(detail, name=ctx.obj['name'])


@cli.group('mongo')
@click.option('--name', '-n', default='mongo', help='Name of the mongo container')
@click.pass_context
def mongo(ctx, name):
    ctx.ensure_object(dict)
    ctx.obj['name'] = name


@mongo.command()
@click.pass_context
def init(ctx):
    name = ctx.obj['name']

    try:
        client.images.get('mongo:latest')
        click.echo('{} is already existed, no need to fetch again'.format(
            green('mongo:latest')))
    except docker.errors.ImageNotFound:
        click.echo('pulling latest mongo image...')
        client.images.pull('mongo', 'latest')
        click.echo('pulled latest mongo image')

    volume_name = '{}-data-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('{} volume is already existed, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('{} volume is created'.format(green(volume_name)))


@mongo.command()
@click.option('--net', '-n', default='evt-net', help='Name of the network for the environment')
@click.option('--port', '-p', default=27017, help='Expose port for mongodb')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for mongodb')
@click.pass_context
def create(ctx, net, port, host):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        client.images.get('mongo:latest')
        client.networks.get(net)
        client.volumes.get(volume_name)
    except docker.errors.ImageNotFound:
        click.echo(
            'Some necessary elements are not found, please run `mongo init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `mongo init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is existed but not running, try to remove old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                '{} container is already existed and running, cannot restart, run `mongo stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    client.containers.create('mongo:latest', None, name=name, detach=True, network=net,
                             ports={'27017/tcp': (host, port)},
                             volumes={volume_name: {
                                 'bind': '/data/db', 'mode': 'rw'}},
                             )
    click.echo('{} container is created'.format(green(name)))


@mongo.command()
@click.option('--all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, cannot clean'.format(green(name)))
            return

        container.remove()
        click.echo('{} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove()
        click.echo('{} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('{} volume is not existed'.format(green(volume_name)))


@mongo.command('start')
@click.pass_context
def startmongo(ctx):
    ctx.invoke(start, name=ctx.obj['name'])


@mongo.command('stop')
@click.pass_context
def stopmongo(ctx):
    ctx.invoke(stop, name=ctx.obj['name'])


@mongo.command('logs')
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
@click.pass_context
def mongologs(ctx, tail, stream):
    ctx.forward(logs, name=ctx.obj['name'])


@mongo.command('detail')
@click.pass_context
def detailmongo(ctx):
    ctx.invoke(detail, name=ctx.obj['name'])


def check_evt_image():
    try:
        client.images.get('everitoken/evt:latest')
    except docker.errors.ImageNotFound:
        click.echo('Cannot find image: {}, please pull first'.format(green('everitoken/evt:latest')))


@cli.group()
@click.option('--name', '-n', default='evtd', help='Name of the container running evtd')
@click.pass_context
def evtd(ctx, name):
    ctx.ensure_object(dict)
    ctx.obj['name'] = name


@evtd.command()
@click.pass_context
def init(ctx):
    name = ctx.obj['name']

    check_evt_image();

    volume_name = '{}-data-volume'.format(name)
    volume2_name = '{}-snapshots-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('{} volume is already existed, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('{} volume is created'.format(green(volume_name)))

    try:
        client.volumes.get(volume2_name)
        click.echo('{} volume is already existed, no need to create one'.
                   format(green(volume2_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume2_name)
        click.echo('{} volume is created'.format(green(volume2_name)))


@evtd.command('export', help='Export reversible blocks to one backup file')
@click.option('--file', '-f', default='rev-{}.logs'.format(datetime.now().strftime('%Y-%m-%d')), help='Backup file name of reversible blocks')
@click.pass_context
def exportrb(ctx, file):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, cannot export reversible blocks'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))
        return

    try:
        client.volumes.get(volume_name)
    except docker.errors.NotFound:
        click.echo('{} volume is not existed'.format(green(volume_name)))
        return

    image = container.image
    folder = '/opt/evt/data/reversible'

    command = '/bin/bash -c \'mkdir -p {0} && /opt/evt/bin/evtd.sh --export-reversible-blocks={0}/{1}\''.format(
        folder, file)
    container = client.containers.run(image, command, detach=True,
                                      volumes={volume_name: {'bind': '/opt/evt/data', 'mode': 'rw'}})
    container.wait()
    logs = container.logs().decode('utf-8')
    if 'node_management_success' in logs:
        click.echo('export reversible blocks successfully\n')
        click.echo(container.logs())
    else:
        click.echo('export reversible blocks failed\n')
        click.echo(container.logs())


@evtd.command('import', help='Import reversible blocks from backup file')
@click.option('--file', '-f', default='rev-{}.logs'.format(datetime.now().strftime('%Y-%m-%d')), help='Backup file name of reversible blocks')
@click.pass_context
def importrb(ctx, file):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, cannot import reversible blocks'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))
        return

    try:
        client.volumes.get(volume_name)
    except docker.errors.NotFound:
        click.echo('{} volume is not existed'.format(green(volume_name)))
        return

    image = container.image
    folder = '/opt/evt/data/reversible'

    command = 'evtd.sh --import-reversible-blocks={0}/{1}'.format(folder, file)
    container = client.containers.run(image, command, detach=True,
                                      volumes={volume_name: {'bind': '/opt/evt/data', 'mode': 'rw'}})
    container.wait()
    logs = container.logs().decode('utf-8')
    if 'node_management_success' in logs:
        click.echo('import reversible blocks successfully\n')
        click.echo(container.logs())
    else:
        click.echo('import reversible blocks failed\n')
        click.echo(container.logs())


@evtd.command()
@click.option('--all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    volume2_name = '{}-snapshots-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, cannot clean'.format(green(name)))
            return

        container.remove()
        click.echo('{} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove()
        click.echo('{} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('{} volume is not existed'.format(green(volume_name)))

    try:
        volume = client.volumes.get(volume2_name)
        volume.remove()
        click.echo('{} volume is removed'.format(green(volume2_name)))
    except docker.errors.NotFound:
        click.echo('{} volume is not existed'.format(green(volume2_name)))


@evtd.command()
@click.argument('arguments', nargs=-1)
@click.option('--type', '-t', default='testnet', type=click.Choice(['testnet', 'mainnet']), help='Type of the image')
@click.option('--net', '-n', default='evt-net', help='Name of the network for the environment')
@click.option('--http-port', '-p', default=8888, help='Expose port for rpc request, set 0 for not expose')
@click.option('--p2p-port', default=7888, help='Expose port for p2p network, set 0 for not expose')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for evtd')
@click.option('--postgres-name', '-g', default='pg', help='Container name or host address of postgres')
@click.option('--postgres-port', default=5432, help='Port of postgres')
@click.option('--postgres-db', default=None, help='Name of database in postgres, if set, postgres and history plugins will be enabled')
@click.pass_context
def create(ctx, net, http_port, p2p_port, host, postgres_name, postgres_port, postgres_db, type, arguments):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    volume2_name = '{}-snapshots-volume'.format(name)

    if type == 'testnet':
        image = 'everitoken/evt:latest'
    else:
        image = 'everitoken/evt-mainnet:latest'

    try:
        client.images.get(image)
        client.networks.get(net)
        client.volumes.get(volume_name)
        client.volumes.get(volume2_name)
    except docker.errors.ImageNotFound:
        click.echo(
            'Some necessary elements are not found, please run `evtd init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `evtd init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is existed but not running, try to remove old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                '{} container is already existed and running, cannot restart, run `evtd stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    entry = 'evtd.sh --http-server-address=0.0.0.0:8888 --p2p-listen-endpoint=0.0.0.0:7888'
    if postgres_db is not None:
        try:
            container = client.containers.get(postgres_name)
            if container.status != 'running':
                click.echo('{} container is not running, please run it first'.format(
                    green(postgres_name)))
                return
        except docker.errors.NotFound:
            click.echo('{} container is not existed, please run `mongo create` first'.format(
                green(postgres_name)))
            return

        click.echo('{}, {}, {} are enabled'.format(green('postgres_plugin'), green(
            'history_plugin'), green('history_api_plugin')))
        entry += ' --plugin=evt::postgres_plugin --plugin=evt::history_plugin --plugin=evt::history_api_plugin'
        entry += ' --postgres-uri=postgresql://postgres@{}:{}/{}'.format(
            postgres_name, postgres_port, postgres_db)
    if arguments is not None and len(arguments) > 0:
        entry += ' ' + ' '.join(arguments)

    ports = {}
    if http_port != 0:
        ports['8888/tcp'] = (host, http_port)
    if p2p_port != 0:
        ports['7888/tcp'] = (host, p2p_port)
    client.containers.create(image, None, name=name, detach=True, network=net,
                             ports=ports,
                             volumes={volume_name: {
                                 'bind': '/opt/evt/data', 'mode': 'rw'},
                                 volume2_name: {
                                 'bind': '/opt/evt/snapshots', 'mode': 'rw'},
                             },
                             entrypoint=entry
                             )
    click.echo('{} container is created'.format(green(name)))


@evtd.command('start')
@click.pass_context
def startevtd(ctx):
    ctx.invoke(start, name=ctx.obj['name'])


@evtd.command('stop')
@click.pass_context
def stopevtd(ctx):
    ctx.invoke(stop, name=ctx.obj['name'])


@evtd.command('logs')
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
@click.pass_context
def evtdlogs(ctx, tail, stream):
    ctx.forward(logs, name=ctx.obj['name'])


@evtd.command('detail')
@click.pass_context
def detailevtd(ctx):
    ctx.invoke(detail, name=ctx.obj['name'])


@cli.group()
@click.option('--name', '-n', default='evtwd', help='Name of the container running evtwd')
@click.pass_context
def evtwd(ctx, name):
    ctx.ensure_object(dict)
    ctx.obj['name'] = name


@evtwd.command()
@click.pass_context
def init(ctx):
    name = ctx.obj['name']

    check_evt_image();

    volume_name = '{}-data-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('{} volume is already existed, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('{} volume is created'.format(green(volume_name)))


@evtwd.command()
@click.pass_context
def create(ctx):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        client.images.get('everitoken/evt:latest')
        client.volumes.get(volume_name)
    except docker.errors.ImageNotFound:
        click.echo(
            'Some necessary elements are not found, please run `evtwd init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `evtwd init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is existed but not running, try to remove old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                '{} container is already existed and running, cannot restart, run `evtwd stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    entry = 'evtwd.sh --unix-socket-path=evtwd.sock'
    client.containers.create('everitoken/evt:latest', None, name=name, detach=True,
                             volumes={volume_name: {
                                 'bind': '/opt/evt/data', 'mode': 'rw'}},
                             entrypoint=entry
                             )
    click.echo('{} container is created'.format(green(name)))


@evtwd.command()
@click.option('--all/--no-all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, cannot clean'.format(green(name)))
            return

        container.remove()
        click.echo('{} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container is not existed'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove(force=True)
        click.echo('{} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('{} volume is not existed'.format(green(volume_name)))


@evtwd.command('start')
@click.pass_context
def startevtwd(ctx):
    ctx.invoke(start, name=ctx.obj['name'])


@evtwd.command('stop')
@click.pass_context
def stopevtwd(ctx):
    ctx.invoke(stop, name=ctx.obj['name'])


@evtwd.command('logs')
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
@click.pass_context
def evtwdlogs(ctx, tail, stream):
    ctx.forward(logs, name=ctx.obj['name'])


@evtwd.command('detail')
@click.pass_context
def detailevtwd(ctx):
    ctx.invoke(detail, name=ctx.obj['name'])


@cli.command(context_settings=dict(
    ignore_unknown_options=True,
    help_option_names=[]
))
@click.argument('commands', nargs=-1, type=click.UNPROCESSED)
@click.option('--evtwd', '-w', default='evtwd', help='Name of evtwd container')
def evtc(commands, evtwd):
    try:
        container = client.containers.get(evtwd)
    except docker.errors.ImageNotFound:
        click.echo(
            'evtc: Some necessary elements are not found, please run `evtwd init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `evtwd init` first')
        return

    entry = '/opt/evt/bin/evtc --wallet-url=unix://opt/evt/data/evtwd.sock {}'.format(' '.join(commands))
    code, result = container.exec_run(entry, stream=True)

    for line in result:
        click.echo(line.decode('utf-8'), nl=False)


if __name__ == '__main__':
    cli()
