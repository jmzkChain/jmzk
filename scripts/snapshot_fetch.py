#!/usr/bin/env python3

import time

import boto3
import click


@click.command()
@click.option('--name', '-n', required=True)
@click.option('--bucket', '-b', default='evt-snapshots')
@click.option('--file', '-f', required=True)
def fetch(name, bucket, file, aws_key, aws_secret):
    s3 = boto3.resource('s3')

    t = time.monotonic()

    click.echo('Downloading {} from {} bucket'.format(
        click.style(name, fg='red'), click.style(bucket, fg='green')))

    try:
        s3.Bucket(bucket).download_file(name, file)
    except botocore.exceptions.ClientError as e:
        if e.response['Error']['Code'] == "404":
            print("The object does not exist.")
        else:
            raise

    click.echo('Download finished, took {} ms'.format(
               click.style(str(round((t2-t) * 1000)), fg='green')))


if __name__ == '__main__':
    fetch()
