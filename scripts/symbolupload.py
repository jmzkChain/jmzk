#!/usr/bin/env python3

import os
import pathlib
import time

import boto3
import click


def get_files(folder):
    files = []
    for it in os.scandir(folder):
        assert it.is_dir()
        for it2 in os.scandir(it.path):
            assert it2.is_file()
            files.append(it2.path)
    return files


@click.command()
@click.option('--folder', '-f', type=click.Path(exists=True), required=True)
@click.option('--bucket', '-b', default='evt-symbols')
@click.option('--ref', '-r', default='evt')
@click.option('--aws-key', '-k', required=True)
@click.option('--aws-secret', '-s', required=True)
def upload(folder, bucket, ref, aws_key, aws_secret):
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
        s3.Object(bucket, key).put(ACL='public-read', Body=open(f, 'rb'), StorageClass='STANDARD_IA')
    t2 = time.monotonic()

    click.echo('Uploaded all the symbols, took {} ms'.format(
               click.style(str(round((t2-t) * 1000)), fg='green')))


if __name__ == '__main__':
    upload()
