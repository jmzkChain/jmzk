#!/usr/bin/env python3

import json
import pathlib
import re
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
        click.echo('{} container does not exist'.format(green(name)))


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
            '{} container does not exist, please start it first'.format(green(name)))


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
            '{} container does not exist, please start it first'.format(green(name)))


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
        if 'Name' in v:
            volumes.append('{}->{}'.format(v['Name'], v['Destination']))
        else:
            volumes.append('{}->{}'.format(v['Source'], v['Destination']))

    click.echo('      id: {}'.format(green(ct['Id'])))
    click.echo('   image: {}'.format(green(ct['Image'])))
    click.echo('image-id: {}'.format(green(ct['ImageID'])))
    click.echo(' command: {}'.format(green(ct['Command'])))
    click.echo(' network: {}'.format(
        green(list(ct['NetworkSettings']['Networks'].keys())[0])))
    click.echo('   ports: {}'.format(green(', '.join(ports))))
    click.echo(' volumes: {}'.format(green(', '.join(volumes))))
    click.echo('  status: {}'.format(green(ct['Status'])))


@cli.command()
@click.option('--prefix', '-p', help='Prefix of snapshots to list')
def snapshots(prefix):
    try:
        client.images.get('jmzkChain/snapshot:latest')
    except docker.errors.ImageNotFound:
        click.echo('Pulling latest snapshot image...')
        client.images.pull('jmzkChain/snapshot', 'latest')
        click.echo('Pulled latest snapshot image')

    if prefix is not None:
        entry = 'list -p {}'.format(prefix)
    else:
        entry = 'list'

    container = client.containers.run('jmzkChain/snapshot:latest', entry, detach=True)
    container.wait()
    logs = container.logs().decode('utf-8')

    container.remove()
    click.echo(logs)


@cli.group('network')
@click.option('--name', '-n', default='jmzk-net', help='Name of the network for the environment')
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
            'Network: {} network already exists, no need to create one'.format(green(name)))
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
        click.echo('Network: {} network does not exist'.format(green(name)))


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
        client.images.get('jmzkChain/postgres:latest')
        click.echo('{} already exists, no need to fetch it again'.format(
            green('jmzkChain/postgres:latest')))
    except docker.errors.ImageNotFound:
        click.echo('Pulling latest postgres image...')
        client.images.pull('jmzkChain/postgres', 'latest')
        click.echo('Pulled latest postgres image')

    volume_name = '{}-data-volume'.format(name)

    try:
        client.volumes.get(volume_name)
        click.echo('{} volume already exists, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('{} volume is created'.format(green(volume_name)))


@postgres.command()
@click.option('--data-volume', '-d', default=None, help='Set one host path for the data folder in postgres instead using volume')
@click.pass_context
def upgrade(ctx, data_volume):
    name = ctx.obj['name']
    image = 'jmzkChain/postgres:latest'
    volume_name = '{}-data-volume'.format(name)
    volume2_name = '{}-config-volume'.format(name)

    try:
        client.images.get(image)
        if data_volume is None:
            client.volumes.get(volume_name)
        client.volumes.get(volume2_name)
    except docker.errors.ImageNotFound:
        click.echo(
            'Some necessary elements are not found, please run `postgres init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `postgres init` first')
        return

    try:
        cmd = "bash -c 'if [ -s '/data/postgresql/data/PG_VERSION' ]; then shopt -s dotglob nullglob && mv -v /data/postgresql/data/* /data/; fi'"
        logs = client.containers.run(image, cmd, auto_remove=True, volumes={volume_name: {'bind': '/data', 'mode': 'rw'}}, stream=True)
    except docker.errors.ContainerError:
        click.echo('Upgrade postgres failed')
        return

    count = 0
    for line in logs:
        click.echo(line, nl=False)
        count += 1

    try:
        cmd = "bash -c 'if [ -s '/config/pg_hba.conf' ]; then cp -v /config/pg_hba.conf /data; fi'"
        logs = client.containers.run(image, cmd, auto_remove=True,
            volumes={
                volume_name: {'bind': '/data', 'mode': 'rw'},
                volume2_name: {'bind': '/config', 'mode': 'rw'}
            }, stream=True)
    except docker.errors.ContainerError:
        click.echo('Upgrade postgres failed')
        return

    for line in logs:
        click.echo(line, nl=False)
        count += 1

    if count > 0:
        click.echo('Upgraded postgres')
    else:
        click.echo("It's no need to upgrade postgres now")


@postgres.command()
@click.option('--net', '-n', default='jmzk-net', help='Name of the network for the environment')
@click.option('--port', '-p', default=5432, help='Expose port for postgres')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for postgres')
@click.option('--password', '-x', default='', help='Password for \'postgres\' user, leave empty to disable password (anyone can login)')
@click.option('--data-volume', '-d', default=None, help='Set one host path for data folder in postgres instead using volume')
@click.option('--shm-size', default='1g', help='Shared memory size for container')
@click.pass_context
def create(ctx, net, port, host, password, data_volume, shm_size):
    name = ctx.obj['name']
    image = 'jmzkChain/postgres:latest'
    volume_name = '{}-data-volume'.format(name)

    if data_volume is not None:
        volume_name = data_volume

    try:
        client.images.get(image)
        client.networks.get(net)
        if data_volume is None:
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
                '{} container exists but not running, try to remove the old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                '{} container already exists and is running, cannot restart, run `postgres stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    if len(password) > 0:
        auth = 'md5'
    else:
        auth = 'trust'

    client.containers.create(image, None, name=name, detach=True, network=net,
                             ports={'5432/tcp': (host, port)},
                             volumes={
                                 volume_name: {
                                     'bind': '/var/lib/postgresql/data', 'mode': 'rw'
                                 }
                             },
                             environment={
                                 'AUTH': auth,
                                 'POSTGRES_PASSWORD': password
                             },
                             shm_size=shm_size)
    click.echo('{} container is created'.format(green(name)))

    if len(password) > 0:
        click.echo('{}: Password is set only if it\'s the first time creating postgres, otherwise it will reuse old password. Please use {} command.'.format(
            green('NOTICE'), green('postgres updpass')))


@postgres.command()
@click.argument('dbname', default='jmzk')
@click.pass_context
def createdb(ctx, dbname):
    name = ctx.obj['name']

    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is not running, cannot create database'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))
        return

    cmd1 = 'psql -U postgres -c "SELECT EXISTS(SELECT datname FROM pg_catalog.pg_database WHERE datname=\'{}\');"'.format(
        dbname)
    c1, logs1 = container.exec_run(cmd1)

    if 't\n' in logs1.decode('utf-8'):
        click.echo(
            '{} database is already created, skip creation'.format(green(dbname)))
        return

    cmd2 = "psql -U postgres -c 'CREATE DATABASE {};'".format(dbname)
    c2, logs2 = container.exec_run(cmd2)

    if 'CREATE DATABASE' in logs2.decode('utf-8'):
        click.echo('Created database: {} in postgres'.format(green(dbname)))


@postgres.command()
@click.argument('new-password', default='')
@click.pass_context
def updpass(ctx, new_password):
    name = ctx.obj['name']

    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is not running, failed to create database'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))
        return

    cmd = 'psql -U postgres -c "ALTER USER postgres WITH PASSWORD \'{}\';"'.format(
        new_password)
    c, logs = container.exec_run(cmd)

    if 'ALTER ROLE' in logs.decode('utf-8'):
        click.echo('Update password successfully')


@postgres.command()
@click.option('--all/--no-all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, failed to clean'.format(green(name)))
            return

        container.remove()
        click.echo('{} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove()
        click.echo('{} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('{} volume does not exist'.format(green(volume_name)))


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


def check_jmzk_image():
    missing_jmzk = False
    missing_jmzk_staking = False
    missing_jmzk_mainnet = False
    try:
        client.images.get('jmzkChain/jmzk:latest')
    except docker.errors.ImageNotFound:
        missing_jmzk = True

    try:
        client.images.get('jmzkChain/jmzk-mainnet:latest')
    except docker.errors.ImageNotFound:
        missing_jmzk_mainnet = True

    try:
        client.images.get('jmzkChain/jmzk-staking:latest')
    except docker.errors.ImageNotFound:
        missing_jmzk_staking = True

    if missing_jmzk and missing_jmzk_mainnet and missing_jmzk_staking:
        click.echo('Nither find image: {}, {} and {}, please pull one first'.format(
            green('jmzkChain/jmzk:latest'), green('jmzkChain/jmzk-mainnet:latest'),  green('jmzkChain/jmzk-staking:latest')))


@cli.group()
@click.option('--name', '-n', default='jmzkd', help='Name of the container running jmzkd')
@click.pass_context
def jmzkd(ctx, name):
    ctx.ensure_object(dict)
    ctx.obj['name'] = name


@jmzkd.command()
@click.pass_context
def init(ctx):
    name = ctx.obj['name']

    check_jmzk_image()

    volume_name = '{}-data-volume'.format(name)
    volume2_name = '{}-snapshots-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('{} volume already exists, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('{} volume is created'.format(green(volume_name)))

    try:
        client.volumes.get(volume2_name)
        click.echo('{} volume already exists, no need to create one'.
                   format(green(volume2_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume2_name)
        click.echo('{} volume is created'.format(green(volume2_name)))


@jmzkd.command('export', help='Export reversible blocks to one backup file')
@click.option('--file', '-f', default='rev-{}.logs'.format(datetime.now().strftime('%Y-%m-%d')), help='Backup file name of reversible blocks')
@click.option('--data-volume', '-d', default=None, help='Set one host path for data folder in jmzkd instead using volume')
@click.pass_context
def exportrb(ctx, file, data_volume):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    if data_volume is not None:
        volume_name = data_volume

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, failed to export reversible blocks'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))
        return

    try:
        if data_volume is None:
            client.volumes.get(volume_name)
    except docker.errors.NotFound:
        click.echo('{} volume does not exist'.format(green(volume_name)))
        return

    image = container.image
    folder = '/opt/jmzk/data/reversible'

    command = '/bin/bash -c \'mkdir -p {0} && /opt/jmzk/bin/jmzkd.sh --export-reversible-blocks={0}/{1}\''.format(
        folder, file)
    container = client.containers.run(image, command, detach=True,
                                      volumes={volume_name: {'bind': '/opt/jmzk/data', 'mode': 'rw'}})
    container.wait()
    logs = container.logs().decode('utf-8')
    if 'node_management_success' in logs:
        click.echo('export reversible blocks successfully\n')
        click.echo(container.logs())
    else:
        click.echo('export reversible blocks failed\n')
        click.echo(container.logs())


@jmzkd.command('import', help='Import reversible blocks from backup file')
@click.option('--file', '-f', default='rev-{}.logs'.format(datetime.now().strftime('%Y-%m-%d')), help='Backup file name of reversible blocks')
@click.option('--data-volume', '-d', default=None, help='Set one host path for data folder in jmzkd instead using volume')
@click.pass_context
def importrb(ctx, file, data_volume):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    if data_volume is not None:
        volume_name = data_volume

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, cannot import reversible blocks'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))
        return

    try:
        if data_volume is None:
            client.volumes.get(volume_name)
    except docker.errors.NotFound:
        click.echo('{} volume does not exist'.format(green(volume_name)))
        return

    image = container.image
    folder = '/opt/jmzk/data/reversible'

    command = 'jmzkd.sh --import-reversible-blocks={0}/{1}'.format(folder, file)
    container = client.containers.run(image, command, detach=True,
                                      volumes={volume_name: {'bind': '/opt/jmzk/data', 'mode': 'rw'}})
    container.wait()
    logs = container.logs().decode('utf-8')
    if 'node_management_success' in logs:
        click.echo('import reversible blocks successfully\n')
        click.echo(container.logs())
    else:
        click.echo('import reversible blocks failed\n')
        click.echo(container.logs())


@jmzkd.command()
@click.option('--all/--no-all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    volume2_name = '{}-snapshots-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, failed to clean'.format(green(name)))
            return

        container.remove()
        click.echo('{} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove()
        click.echo('{} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('{} volume does not exist'.format(green(volume_name)))

    try:
        volume = client.volumes.get(volume2_name)
        volume.remove()
        click.echo('{} volume is removed'.format(green(volume2_name)))
    except docker.errors.NotFound:
        click.echo('{} volume does not exist'.format(green(volume2_name)))


@jmzkd.command()
@click.option('--postgres/--no-postgres', '-p', default=False, help='Whether export postgres data into snapshot (postgres plugin should be enabled)')
@click.option('--upload/--no-upload', '-u', default=False, help='Whether upload to S3')
@click.option('--aws-key', '-k', default='')
@click.option('--aws-secret', '-s', default='')
@click.pass_context
def snapshot(ctx, postgres, upload, aws_key, aws_secret):
    name = ctx.obj['name']
    image = 'jmzkChain/snapshot:latest'
    volume_name = '{}-snapshots-volume'.format(name)

    try:
        client.images.get(image)
    except docker.errors.ImageNotFound:
        click.echo('Pulling latest snapshot image...')
        client.images.pull('jmzkChain/snapshot', 'latest')
        click.echo('Pulled latest snapshot image')

    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container is not running, failed to create snapshot'.format(green(name)))
            return
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))
        return

    if postgres:
        p = '-p'
    else:
        p = ''

    entry = '/opt/jmzk/bin/jmzkc -u unix:///opt/jmzk/data/jmzkd.sock producer snapshot {}'.format(
        p)
    code, result = container.exec_run(entry)

    obj = {}
    pat = re.compile(r'\|->([\w_]+) : (.*)')
    it = pat.finditer(result.decode('utf-8'))
    for m in it:
        obj[m.group(1)] = m.group(2)

    if len(obj) == 0:
        click.echo('Take snapshot failed, please make sure producer_api_plugin is enabled.')
        return

    click.echo(json.dumps(obj, indent=2))

    if not upload:
        return

    if aws_key == '' or aws_secret == '':
        click.echo('AWS key or secret is empty, cannot upload to S3')
        return

    if obj['postgres'] == 'true':
        pg = '--postgres'
    else:
        pg = '--no-postgres'

    entry = "upload --file=/data/{} --block-id='{}' --block-num={} --block-time='{}' {} --aws-key={} --aws-secret={}".format(
        pathlib.Path(obj['snapshot_name']).name, obj['head_block_id'], obj['head_block_num'], obj['head_block_time'], pg, aws_key, aws_secret)

    container = client.containers.run(image, entry, detach=True,
                                      volumes={volume_name: {'bind': '/data', 'mode': 'rw'}},)
    container.wait()
    logs = container.logs().decode('utf-8')

    click.echo(logs)
    container.remove()


@jmzkd.command()
@click.argument('snapshot')
@click.pass_context
def getsnapshot(ctx, snapshot):
    name = ctx.obj['name']
    image = 'jmzkChain/snapshot:latest'
    volume_name = '{}-snapshots-volume'.format(name)

    try:
        client.images.get(image)
    except docker.errors.ImageNotFound:
        click.echo('Pulling latest snapshot image...')
        client.images.pull('jmzkChain/snapshot', 'latest')
        click.echo('Pulled latest snapshot image')

    sid = snapshot[8:]
    entry = 'fetch --name={} --file=/data/{}'.format(snapshot, sid)

    container = client.containers.run('jmzkChain/snapshot:latest', entry, detach=True,
                                      volumes={volume_name: {'bind': '/data', 'mode': 'rw'}})
    container.wait()
    logs = container.logs().decode('utf-8')

    container.remove()
    click.echo(logs, nl=False)
    click.echo('Create jmzkd with this snapshot via \'--snapshot=/opt/jmzk/snapshots/{}\''.format(sid))


@jmzkd.command()
@click.argument('arguments', nargs=-1)
@click.option('--type', '-t', default='testnet', type=click.Choice(['testnet', 'mainnet', 'staking']), help='Type of the image')
@click.option('--net', '-n', default='jmzk-net', help='Name of the network for the environment')
@click.option('--http-port', '-p', default=8888, help='Expose port for rpc request, set 0 for not expose')
@click.option('--p2p-port', default=7888, help='Expose port for p2p network, set 0 for not expose')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for jmzkd')
@click.option('--postgres-name', '-g', default='pg', help='Container name or host address of postgres')
@click.option('--postgres-db', default=None, help='Name of database in postgres, if set, postgres and history plugins will be enabled')
@click.option('--postgres-pass', default='', help='Password for postgres')
@click.option('--data-volume', '-d', default=None, help='Set one host path for data folder in jmzkd instead using volume')
@click.option('--shm-size', default='1g', help='Shared memory size for container')
@click.pass_context
def create(ctx, net, http_port, p2p_port, host, postgres_name, postgres_db, postgres_pass, data_volume, type, shm_size, arguments):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)
    volume2_name = '{}-snapshots-volume'.format(name)
    if data_volume is not None:
        volume_name = data_volume

    if type == 'testnet':
        image = 'jmzkChain/jmzk:latest'
    elif type == 'mainnet':
        image = 'jmzkChain/jmzk-mainnet:latest'
    elif type == 'staking':
        image = 'jmzkChain/jmzk-staking:latest'
    else:
        click.echo('Unknown image type')
        return

    try:
        client.images.get(image)
        client.networks.get(net)
        if data_volume is None:
            client.volumes.get(volume_name)
        client.volumes.get(volume2_name)
    except docker.errors.ImageNotFound:
        click.echo(
            '{} image is not found, please use docker pull image first'.format(image))
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `jmzkd init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container exists but not running, try to remove the old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                '{} container already exists and is running, cannot restart, run `jmzkd stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    entry = 'jmzkd.sh --http-server-address=0.0.0.0:8888 --p2p-listen-endpoint=0.0.0.0:7888'
    if postgres_db is not None:
        try:
            container = client.containers.get(postgres_name)
            if container.status != 'running':
                click.echo('{} container is not running, please run it first'.format(
                    green(postgres_name)))
                return
        except docker.errors.NotFound:
            click.echo('{} container does not exist, please run `postgres create` first'.format(
                green(postgres_name)))
            return

        click.echo('{}, {}, {} are enabled'.format(green('postgres_plugin'), green(
            'history_plugin'), green('history_api_plugin')))
        entry += ' --plugin=jmzk::postgres_plugin --plugin=jmzk::history_plugin --plugin=jmzk::history_api_plugin'

        if len(postgres_pass) == 0:
            entry += ' --postgres-uri=postgresql://postgres@{}:{}/{}'.format(
                postgres_name, 5432, postgres_db)
        else:
            entry += ' --postgres-uri=postgresql://postgres:{}@{}:{}/{}'.format(
                postgres_pass, postgres_name, 5432, postgres_db)

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
                                 'bind': '/opt/jmzk/data', 'mode': 'rw'},
                                 volume2_name: {
                                 'bind': '/opt/jmzk/snapshots', 'mode': 'rw'},
                             },
                             entrypoint=entry,
                             cap_add=['SYS_PTRACE'],
                             security_opt=['apparmor:unconfined'],
                             shm_size=shm_size
                             )
    click.echo('{} container is created'.format(green(name)))


@jmzkd.command('start')
@click.pass_context
def startjmzkd(ctx):
    ctx.invoke(start, name=ctx.obj['name'])


@jmzkd.command('stop')
@click.pass_context
def stopjmzkd(ctx):
    ctx.invoke(stop, name=ctx.obj['name'])


@jmzkd.command('logs')
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
@click.pass_context
def jmzkdlogs(ctx, tail, stream):
    ctx.forward(logs, name=ctx.obj['name'])


@jmzkd.command('detail')
@click.pass_context
def detailjmzkd(ctx):
    ctx.invoke(detail, name=ctx.obj['name'])


@cli.group()
@click.option('--name', '-n', default='jmzkwd', help='Name of the container running jmzkwd')
@click.pass_context
def jmzkwd(ctx, name):
    ctx.ensure_object(dict)
    ctx.obj['name'] = name


@jmzkwd.command()
@click.pass_context
def init(ctx):
    name = ctx.obj['name']

    check_jmzk_image()

    volume_name = '{}-data-volume'.format(name)
    try:
        client.volumes.get(volume_name)
        click.echo('{} volume already exists, no need to create one'.
                   format(green(volume_name)))
    except docker.errors.NotFound:
        client.volumes.create(volume_name)
        click.echo('{} volume is created'.format(green(volume_name)))


@jmzkwd.command()
@click.argument('arguments', nargs=-1)
@click.option('--net', '-n', default='jmzk-net', help='Name of the network for the environment')
@click.option('--type', '-t', default='testnet', type=click.Choice(['testnet', 'mainnet', 'staking']), help='Type of the image')
@click.option('--http/--no-http', default=False, help='Whether to enable http server')
@click.option('--host', '-h', default='127.0.0.1', help='Host address for jmzkwd (only works when http is enabled)')
@click.option('--http-port', '-p', default=9999, help='Expose port for rpc request, set 0 for not expose (only works when http is enabled)')
@click.pass_context
def create(ctx, net, type, http, host, http_port, arguments):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    if type == 'testnet':
        image = 'jmzkChain/jmzk:latest'
    elif type == 'mainnet':
        image = 'jmzkChain/jmzk-mainnet:latest'
    elif type == 'staking':
        image = 'jmzkChain/jmzk-staking:latest'
    else:
        click.echo('Unknown image type')
        return

    try:
        client.images.get(image)
        client.volumes.get(volume_name)
    except docker.errors.ImageNotFound:
        click.echo(
            '{} image is not found, please use docker pull image first'.format(image))
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `jmzkwd init` first')
        return

    create = False
    try:
        container = client.containers.get(name)
        if container.status != 'running':
            click.echo(
                '{} container exists but not running, try to remove old container and start a new one'.format(green(name)))
            container.remove()
            create = True
        else:
            click.echo(
                '{} container already exists and running, cannot restart, run `jmzkwd stop` first'.format(green(name)))
            return
    except docker.errors.NotFound:
        create = True

    if not create:
        return

    entry = 'jmzkwd.sh --unix-socket-path=/opt/jmzk/data/wallet/jmzkwd.sock'
    ports = {}
    if http:
        entry += ' --listen-http --http-server-address=0.0.0.0:9999'
        if http_port != 0:
            ports['9999/tcp'] = (host, http_port)

    if arguments is not None and len(arguments) > 0:
        entry += ' ' + ' '.join(arguments)

    client.containers.create(image, None, name=name, detach=True,
                             network=net,
                             ports=ports,
                             volumes={volume_name: {
                                 'bind': '/opt/jmzk/data', 'mode': 'rw'}},
                             entrypoint=entry
                             )
    click.echo('{} container is created'.format(green(name)))


@jmzkwd.command()
@click.option('--all/--no-all', '-a', default=False, help='Clear both container and volume, otherwise only clear container')
@click.pass_context
def clear(ctx, all):
    name = ctx.obj['name']
    volume_name = '{}-data-volume'.format(name)

    try:
        container = client.containers.get(name)
        if container.status == 'running':
            click.echo(
                '{} container is still running, failed to clean'.format(green(name)))
            return

        container.remove()
        click.echo('{} container is removed'.format(green(name)))
    except docker.errors.NotFound:
        click.echo('{} container does not exist'.format(green(name)))

    if not all:
        return

    try:
        volume = client.volumes.get(volume_name)
        volume.remove(force=True)
        click.echo('{} volume is removed'.format(green(volume_name)))
    except docker.errors.NotFound:
        click.echo('{} volume does not exist'.format(green(volume_name)))


@jmzkwd.command('start')
@click.pass_context
def startjmzkwd(ctx):
    ctx.invoke(start, name=ctx.obj['name'])


@jmzkwd.command('stop')
@click.pass_context
def stopjmzkwd(ctx):
    ctx.invoke(stop, name=ctx.obj['name'])


@jmzkwd.command('logs')
@click.option('--tail', '-t', default=100, help='Output specified number of lines at the end of logs')
@click.option('--stream/--no-stream', '-s', default=False, help='Stream the output')
@click.pass_context
def jmzkwdlogs(ctx, tail, stream):
    ctx.forward(logs, name=ctx.obj['name'])


@jmzkwd.command('detail')
@click.pass_context
def detailjmzkwd(ctx):
    ctx.invoke(detail, name=ctx.obj['name'])


@cli.command(context_settings=dict(
    ignore_unknown_options=True,
    help_option_names=[]
))
@click.argument('commands', nargs=-1, type=click.UNPROCESSED)
@click.option('--jmzkwd', '-w', default='jmzkwd', help='Name of jmzkwd container')
@click.option('--net', '-n', default='jmzk-net', help='Name of the network for the environment')
def jmzkc(commands, jmzkwd, net):
    import subprocess
    import sys

    try:
        container = client.containers.get(jmzkwd)
    except docker.errors.ImageNotFound:
        click.echo(
            'jmzkc: Some necessary elements are not found, please run `jmzkwd init` first')
        return
    except docker.errors.NotFound:
        click.echo(
            'Some necessary elements are not found, please run `jmzkwd init` first')
        return

    if container.status != 'running':
        click.echo(
            '{} container is not running, please start it first'.format(green('jmzkwd')))
        return

    commands = map(lambda x: '"{}"'.format(x) if ' ' in x else x, commands)
    args = 'docker exec -i {} /opt/jmzk/bin/jmzkc --wallet-url=unix://opt/jmzk/data/wallet/jmzkwd.sock {}'.format(jmzkwd, ' '.join(commands))

    proc = subprocess.Popen(args, stdin=sys.stdin, stdout=sys.stdout, stderr=sys.stdout, shell=True)
    proc.wait()


if __name__ == '__main__':
    cli()
