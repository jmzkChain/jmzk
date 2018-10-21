#!/usr/bin/env python

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
@click.option('--local/--no-local', default=True)
@click.pass_context
def start(ctx, net, port, local):
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

    if local:
        host = '127.0.0.1'
    else:
        host = '0.0.0.0'

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
@click.pass_context
def clean(ctx):
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
@click.option('--file', '-f', default='rev-{}.logs'.format(datetime.now().strftime('%Y-%m-%d')), help='Backup file name of reversible blocks')
@click.pass_context
def export(ctx, file):
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
    folder = "/opt/evt/data/reversible"

    try:
        command = '/bin/bash -c \'mkdir -p {0} && /opt/evt/bin/evtd.sh --export-reversible-blocks={0}/{1}\''.format(folder, file)
        output = client.containers.run(image, command, auto_remove=True,
                                   volumes={volume_name: {'bind': '/opt/evt/data', 'mode': 'rw'}})

        click.echo(output)
    except docker.errors.ContainerError as e:
        click.echo(e)


if __name__ == '__main__':
    cli()
