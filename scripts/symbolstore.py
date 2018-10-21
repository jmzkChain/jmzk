#!/usr/bin/env python

import os
import pathlib
import shutil
import subprocess

import click


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


@click.command()
@click.argument('path', type=click.Path(exists=True))
@click.option('--symbols-folder', '-s', help='Path to the folder of symbols', required=True)
def export(path, symbols_folder):
    if os.path.isdir(path):
        scan_dir(path, symbols_folder)
    else:
        export_symbol(path, symbols_folder)


if __name__ == '__main__':
    export()
