import os
import re
import subprocess

import boto3
import click
from botocore.exceptions import ClientError
from botocore.handlers import disable_signing


@click.command()
@click.argument('filename', type=click.Path(exists=True))
@click.option('--type', '-t', default='testnet')
@click.option('--temp', '-t', default='/tmp/evt-symbols')
@click.option('--output', '-o', default='.')
def analysis(filename, type, temp, output):
    r = subprocess.run(['minidump_stackwalk', filename],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        click.echo('Dump failed')
        click.echo(r.stderr)
        exit(-1)

    match = re.search(r'evtd, (\w+)', r.stdout.decode('utf-8'))
    if match is None:
        click.echo('Cannot find checksum of evtd, failed')
        exit(-1)

    checksum = match.group(1)
    click.echo('Checksum is ' + checksum)

    s3 = boto3.resource('s3')
    s3.meta.client.meta.events.register('choose-signer.s3.*', disable_signing)

    if type == 'testnet':
        folder = 'evt'
    else:
        folder = 'evt-mainnet'

    f1 = '{}/evtd/{}/evtd.dbg'.format(folder, checksum)
    f2 = '{}/evtd/{}/evtd.sym'.format(folder, checksum)

    if not os.path.exists(temp):
        os.mkdir(temp)

    folder1 = os.path.join(temp, 'evtd')
    if not os.path.exists(folder1):
        os.mkdir(folder1)

    folder2 = os.path.join(folder1, checksum)
    if not os.path.exists(folder2):
        os.mkdir(folder2)

    o1 = os.path.join(folder2, 'evtd.dbg')
    o2 = os.path.join(folder2, 'evtd.sym')

    try:
        if not os.path.exists(o1):
            click.echo('Fetching {} to {}...'.format(f1, o1))
            s3.Bucket('evt-symbols').download_file(f1, o1)

        if not os.path.exists(o2):
            click.echo('Fetching {} to {}...'.format(f2, o2))
            s3.Bucket('evt-symbols').download_file(f2, o2)
    except ClientError as e:
        if e.response['Error']['Code'] == '404':
            click.echo(
                'Cannot find symbol files for evtd with checksum: {}.'.format(checksum))
        else:
            raise

    r = subprocess.run(['minidump_stackwalk', filename,
                        temp], stdout=subprocess.PIPE)
    click.echo(r.stdout)

    with open(os.path.join(output, 'crash_{}.log'.format(checksum)), 'w') as writer:
        writer.write(r.stdout.decode('utf-8'))

    click.echo('Saved to {}'.format(os.path.join(
        output, 'crash_{}.log'.format(checksum))))


if __name__ == '__main__':
    analysis()
