#!/usr/bin/env python3

import os
import pathlib
import shutil
import subprocess
import time

import click


def green(text):
    return click.style(text, fg='green')


@click.group('cli')
def cli():
    pass


def export_symbol(file, folder):
    filename = os.path.basename(file)
    sym_file = '{}.sym'.format(filename)
    with open(sym_file, 'wb') as writer:
        subprocess.run(['dump_syms', file], stdout=writer)
    if not os.path.exists(sym_file):
        print('Dump symbols for {} failed'.format(file))
        exit(-1)

    with open(sym_file, 'r') as fs:
        line = fs.readline()
        parts = line.split(' ')
        sym_folder = '{}/{}/{}'.format(folder,
                                       parts[4].replace('\n', ''), parts[3])
        if not os.path.exists(sym_folder):
            os.makedirs(sym_folder)

    shutil.move(sym_file, '{}/{}'.format(sym_folder, sym_file))

    dbg_file = '{}/{}.dbg'.format(sym_folder, filename)
    subprocess.run(['objcopy', '--only-keep-debug',
                    file, dbg_file], check=True)
    subprocess.run(['objcopy', '--strip-debug', file], check=True)
    subprocess.run(['objcopy', '--add-gnu-debuglink=%s' %
                    dbg_file, file], check=True)

    print('Exported symbol from {} to {}'.format(file, dbg_file))


def scan_dir(path, folder):
    for file in os.listdir(path):
        ext = pathlib.Path(file).suffix
        if ext == '.so':
            export_symbol(os.path.join(path, file), folder)


@cli.command()
@click.argument('path', type=click.Path(exists=True))
@click.option('--symbols-folder', '-s', help='Path to the folder of symbols', required=True)
def export(path, symbols_folder):
    if os.path.isdir(path):
        scan_dir(path, symbols_folder)
    else:
        export_symbol(path, symbols_folder)


def get_files(folder):
    files = []
    for it in os.scandir(folder):
        assert it.is_dir()
        for it2 in os.scandir(it.path):
            assert it2.is_file()
            files.append(it2.path)
    return files


@cli.command()
@click.option('--folder', '-f', type=click.Path(exists=True), required=True)
@click.option('--bucket', '-b', default='evt-symbols')
@click.option('--ref', '-r', default='evt')
@click.option('--aws-key', '-k', required=True)
@click.option('--aws-secret', '-s', required=True)
def upload(folder, bucket, ref, aws_key, aws_secret):
    import boto3

    session = boto3.Session(aws_access_key_id=aws_key,
                            aws_secret_access_key=aws_secret)
    s3 = session.resource('s3')

    files = []
    for f in get_files(pathlib.Path(folder) / 'evtd'):
        files.append(f)
    for f in get_files(pathlib.Path(folder) / 'evtc'):
        files.append(f)
    for f in get_files(pathlib.Path(folder) / 'evtwd'):
        files.append(f)

    t = time.monotonic()
    for f in files:
        key = ref + '/' + '/'.join(pathlib.Path(f).parts[-3:])
        click.echo('Uploading: {} to {} bucket'.format(
            click.style(key, fg='red'), click.style(bucket, fg='green')))
        s3.Object(bucket, key).put(ACL='public-read',
                                   Body=open(f, 'rb'), StorageClass='STANDARD_IA')
    t2 = time.monotonic()

    click.echo('Uploaded all the symbols, took {} ms'.format(
               click.style(str(round((t2-t) * 1000)), fg='green')))


if __name__ == '__main__':
    cli()
