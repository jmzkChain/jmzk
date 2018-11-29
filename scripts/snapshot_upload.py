#!/usr/bin/env python3

import os
import pathlib
import time

import boto3
import click


@click.command()
@click.option('--file', '-f', type=click.Path(exists=True), required=True)
@click.option('--block-id', '-i', default='')
@click.option('--block-num', '-n', default='')
@click.option('--block-time', '-t', default='')
@click.option('--bucket', '-b', default='evt-snapshots')
@click.option('--aws-key', '-k', required=True)
@click.option('--aws-secret', '-s', required=True)
def upload(file, block_id, block_num, block_time, bucket, aws_key, aws_secret):
    session = boto3.Session(aws_access_key_id=aws_key,
                            aws_secret_access_key=aws_secret)
    s3 = session.resource('s3')
    key = pathlib.Path(file).name

    t = time.monotonic()

    click.echo('Uploading: {} to {} bucket'.format(
        click.style(key, fg='red'), click.style(bucket, fg='green')))
    s3.Object(bucket, key).put(
        ACL='public-read',
        Body=open(f, 'rb'),
        Metadata={
            'block-id': block_id,
            'block_num': block_num,
            'block_time': block_time
        }
    )

    t2 = time.monotonic()

    click.echo('Uploaded all the symbols, took {} ms'.format(
               click.style(str(round((t2-t) * 1000)), fg='green')))


if __name__ == '__main__':
    upload()
