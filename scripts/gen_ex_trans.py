#!/usr/bin/env python3

import json
import re

import click


@click.command()
@click.argument('input', type=click.Path(exists=True))
@click.argument('output', type=click.Path())
def gen(input, output):
    exs = []

    with open(input, 'r') as reader:
        for line in reader:
            if not line.startswith('FC_DECLARE_DERIVED_EXCEPTION'):
                continue
            result = re.match(
                r'FC_DECLARE_DERIVED_EXCEPTION\(\s+(\w+),\s+(\w+),\s+(\d+),\s+\"(.+)\"\s?\)', line)
            if result:
                exs.append({
                    'name': result.group(1),
                    'code': result.group(3),
                    'en': result.group(4)
                })

    with open(output, 'w') as writer:
        writer.write(json.dumps(exs, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    gen()
