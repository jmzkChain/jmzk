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
        click.echo('Mongo: {} is already existed, no need to fetch again'.format(
            green('mongo:latest')))
    except docker.errors.ImageNotFound:
        click.echo('Mongo: pulling latest mongo image...')
        client.images.pull('mongo', 'latest')
        click.echo('Mongo: pulled latest mongo image')

    volume_name = '{}-data-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('Mongo: {} volume is already existed, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('Mongo: {} volume is created'.format(green(volume_name)))


@mongo.command()
@click.option('--net', '-n', default='evt-net', help='Name of the network for the environment')
@click.option('--port', '-p', default=27017, help='Expose port for mongodb')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for mongodb')
@click.pass_context
def start(ctx, net, port, host):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        client.images.get('mongo:latest')
        client.networks.get(net)
        client.volumes.get(volume_name)
    except docker.errors.ImageNotFound:
        click.echo(
            'Mongo: Some necessary elements are not found, please run `mongo init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Mongo: Some necessary elements are not found, please run `mongo init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                'Mongo: {} container is existed but not running, try to remove old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                'Mongo: {} container is already existed and running, cannot restart, run `mongo stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    client.containers.run('mongo:latest', None, name=name, detach=True, network=net,
                          ports={'27017/tcp': (host, port)},
                          volumes={volume_name: {
                              'bind': '/data/db', 'mode': 'rw'}},
                          )
    click.echo('Mongo: {} container is started'.format(green(name)))


@mongo.command()
@click.pass_context
def stop(ctx):
    name = ctx.obj['name']

    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                'Mongo: {} container is already stopped'.format(green(name)))
            return

        container.stop()
        click.echo('Mongo: {} container is stopped'.format(green(name)))
    except docker.errors.NotFound:
        click.echo(
            'Mongo: {} container is not existed, please start first'.format(green(name)))


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
                'Mongo: {} container is still running, cannot clean'.format(green(name)))
            return

        container.remove()
        click.echo('Mongo: {} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('Mongo: {} container is not existed'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove()
        click.echo('Mongo: {} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('Mongo: {} volume is not existed'.format(green(volume_name)))


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

    volume_name = '{}-data-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('evtd: {} volume is already existed, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('evtd: {} volume is created'.format(green(volume_name)))


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
                'evtd: {} container is still running, cannot export reversible blocks'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('evtd: {} container is not existed'.format(green(name)))
        return

    try:
        client.volumes.get(volume_name)
    except docker.errors.NotFound:
        click.echo('evtd: {} volume is not existed'.format(green(volume_name)))
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
        click.echo('evtd: export reversible blocks successfully\n')
        click.echo(container.logs())
    else:
        click.echo('evtd: export reversible blocks failed\n')
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
                'evtd: {} container is still running, cannot import reversible blocks'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('evtd: {} container is not existed'.format(green(name)))
        return

    try:
        client.volumes.get(volume_name)
    except docker.errors.NotFound:
        click.echo('evtd: {} volume is not existed'.format(green(volume_name)))
        return

    image = container.image
    folder = '/opt/evt/data/reversible'

    command = 'evtd.sh --import-reversible-blocks={0}/{1}'.format(folder, file)
    container = client.containers.run(image, command, detach=True,
                                      volumes={volume_name: {'bind': '/opt/evt/data', 'mode': 'rw'}})
    container.wait()
    logs = container.logs().decode('utf-8')
    if 'node_management_success' in logs:
        click.echo('evtd: import reversible blocks successfully\n')
        click.echo(container.logs())
    else:
        click.echo('evtd: import reversible blocks failed\n')
        click.echo(container.logs())


@evtd.command()
@click.pass_context
def stop(ctx):
    name = ctx.obj['name']

    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                'evtd: {} container is already stopped'.format(green(name)))
            return

        container.stop()
        click.echo('evtd: {} container is stopped'.format(green(name)))
    except docker.errors.NotFound:
        click.echo(
            'evtd: {} container is not existed, please start first'.format(green(name)))


@evtd.command()
@click.option('--all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                'evtd: {} container is still running, cannot clean'.format(green(name)))
            return

        container.remove()
        click.echo('evtd: {} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('evtd: {} container is not existed'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove()
        click.echo('evtd: {} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('evtd: {} volume is not existed'.format(green(volume_name)))


@evtd.command()
@click.argument('arguments', nargs=-1)
@click.option('--type', '-t', default='testnet', type=click.Choice(['testnet', 'mainnet']), help='Type of the image')
@click.option('--net', '-n', default='evt-net', help='Name of the network for the environment')
@click.option('--http-port', '-p', default=8888, help='Expose port for rpc request, set 0 for not expose')
@click.option('--p2p-port', default=7888, help='Expose port for p2p network, set 0 for not expose')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for evtd')
@click.option('--mongodb-name', '-m', default='mongo', help='Container name or host address of mongodb')
@click.option('--mongodb-port', default=27017, help='Port of mongodb')
@click.option('--mongodb-db', default=None, help='Name of database in mongodb, if set, mongo and history plugins will be enabled')
@click.pass_context
def create(ctx, net, http_port, p2p_port, host, mongodb_name, mongodb_port, mongodb_db, type, arguments):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    if type == 'testnet':
        image = 'everitoken/evt:latest'
    else:
        image = 'everitoken/evt-mainnet:latest'

    try:
        client.images.get(image)
        client.networks.get(net)
        client.volumes.get(volume_name)
    except docker.errors.ImageNotFound:
        click.echo(
            'Mongo: Some necessary elements are not found, please run `evtd init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Mongo: Some necessary elements are not found, please run `evtd init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                'evtd: {} container is existed but not running, try to remove old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                'evtd: {} container is already existed and running, cannot restart, run `mongo stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    entry = 'evtd.sh --http-server-address=0.0.0.0:8888 --p2p-listen-endpoint=0.0.0.0:7888'
    if mongodb_db is not None:
        try:
            container = client.containers.get(mongodb_name)
            if container.status != 'running':
                click.echo('{} container is not running, please run it first'.format(
                    green(mongodb_name)))
                return
        except docker.errors.NotFound:
            click.echo('{} container is not existed, please create mongo container first'.format(
                green(mongodb_name)))
            return

        click.echo('{}, {}, {} are enabled'.format(green('mongo_db_plugin'), green(
            'history_plugin'), green('history_api_plugin')))
        entry += ' --plugin=evt::mongo_db_plugin --plugin=evt::history_plugin --plugin=evt::history_api_plugin'
        entry += ' --mongodb-uri=mongodb://{}:{}/{}'.format(
            mongodb_name, mongodb_port, mongodb_db)
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
                                 'bind': '/opt/evt/data', 'mode': 'rw'}},
                             entrypoint=entry
                             )
    click.echo('evtd: {} container is created'.format(green(name)))


@evtd.command()
@click.pass_context
def detail(ctx):
    name = ctx.obj['name']

    client = docker.APIClient()
    containers = client.containers(all=True, filters={'name': name})
    if len(containers) == 0:
        click.echo('evtd: {} container is not found'.format(green(name)))
        return

    ct = containers[0]

    ports = []
    for p in ct['Ports']:
        ports.append('{}:{}->{}/{}'.format(p['IP'], p['PublicPort'], p['PrivatePort'], p['Type']))

    click.echo('      id: {}'.format(green(ct['Id'])))
    click.echo('   image: {}'.format(green(ct['Image'])))
    click.echo('image-id: {}'.format(green(ct['ImageID'])))
    click.echo(' command: {}'.format(green(ct['Command'])))
    click.echo(' network: {}'.format(green(list(ct['NetworkSettings']['Networks'].keys())[0])))
    click.echo('   ports: {}'.format(green(', '.join(ports))))
    click.echo('  status: {}'.format(green(ct['Status'])))


@evtd.command()
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
@click.pass_context
def logs(ctx, tail, stream):
    name = ctx.obj['name']

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
            'evtd: {} container is not existed, please start first'.format(green(name)))


@evtd.command()
@click.pass_context
def start(ctx):
    name = ctx.obj['name']

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                'evtd: {} container is already running, cannot restart'.format(green(name)))
            return

        container.start()
        click.echo('evtd: {} container is started'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('evtd: {} container is not existed'.format(green(name)))


if __name__ == '__main__':
    cli()
