#!/usr/bin/env python3

import datetime
import os
import pathlib
import time

import boto3
import click
from botocore.exceptions import ClientError
from botocore.handlers import disable_signing


def green(text):
    return click.style(text, fg='green')


@click.group('cli')
def cli():
    pass


@cli.command()
@click.option('--file', '-f', type=click.Path(exists=True), required=True)
@click.option('--block-id', '-i', default='')
@click.option('--block-num', '-n', default='')
@click.option('--block-time', '-t', default='')
@click.option('--postgres/--no-postgres', '-p', default=False)
@click.option('--bucket', '-b', default='evt-snapshots')
@click.option('--aws-key', '-k', required=True)
@click.option('--aws-secret', '-s', required=True)
def upload(file, block_id, block_num, block_time, postgres, bucket, aws_key, aws_secret):
    session = boto3.Session(aws_access_key_id=aws_key,
                            aws_secret_access_key=aws_secret)
    s3 = session.resource('s3')

    t = time.monotonic()

    key = '{}/{}'.format(datetime.datetime.now().strftime('%Y-%m'), pathlib.Path(file).name)
    click.echo('Uploading: {} to {} bucket'.format(
        click.style(key, fg='red'), click.style(bucket, fg='green')))

    if postgres:
        pg = 'yes'
    else:
        pg = 'no'

    s3.Object(bucket, key).put(
        Body=open(file, 'rb'),
        Metadata={
            'block-id': block_id,
            'block_num': block_num,
            'block_time': block_time,
            'postgres': pg
        },
        StorageClass='STANDARD_IA'
    )

    s3.ObjectAcl(bucket, key).put(
        ACL='public-read'
    )

    t2 = time.monotonic()

    click.echo('Uploaded snapshot, took {} ms'.format(
               click.style(str(round((t2-t) * 1000)), fg='green')))


@cli.command()
@click.option('--name', '-n', required=True)
@click.option('--bucket', '-b', default='evt-snapshots')
@click.option('--file', '-f', required=True)
def fetch(name, bucket, file):
    s3 = boto3.resource('s3')
    s3.meta.client.meta.events.register('choose-signer.s3.*', disable_signing)

    t = time.monotonic()

    click.echo('Downloading {} from {} bucket'.format(
        click.style(name, fg='red'), click.style(bucket, fg='green')))

    try:
        s3.Bucket(bucket).download_file(name, file)
    except ClientError as e:
        if e.response['Error']['Code'] == '404':
            print('The object does not exist.')
        else:
            raise

    t2 = time.monotonic()

    click.echo('Download to {} finished, took {} ms'.format(
               green(file), click.style(str(round((t2-t) * 1000)), fg='green')))


@cli.command()
@click.option('--prefix', '-p', help='Prefix of snapshots to list')
@click.option('--bucket', '-b', default='evt-snapshots')
def list(prefix, bucket):
    s3 = boto3.resource('s3')
    s3.meta.client.meta.events.register('choose-signer.s3.*', disable_signing)

    if prefix is None:
        prefix = datetime.datetime.now().strftime('%Y-%m')

    click.echo("Querying snapshots with prefix: {}\n".format(green(prefix)))

    objs = s3.Bucket(bucket).objects.filter(Prefix=prefix)

    click.echo('{:<90} {:>12} {:>10} {:>15} {:>25}'.format('name', 'number', 'postgres', 'size', 'time'))
    for obj in objs:
        if len(obj.key) == 8:
            continue

        name = obj.key
        metas = s3.Object(bucket, name).metadata
        size = s3.Object(bucket, name).content_length
        size_str = '{:.3f} MB'.format(size / (1024 * 1024))
        num = int(metas['block_num'])
        time = metas['block_time']
        if 'postgres' in metas:
            pg = metas['postgres']
        else:
            pg = 'NA'

        click.echo('{:<90} {:>12n} {:>10} {:>15} {:>25}'.format(name, num, pg, size_str, time))


if __name__ == '__main__':
    cli()
