import click
import re


@click.command()
@click.argument('input', type=click.Path(exists=True))
@click.argument('output', type=click.Path())
def gen(input, output):
    exs = []

    with open(input, 'r') as reader:
        for line in reader:
            if not line.startswith('FC_DECLARE_DERIVED_EXCEPTION'):
                continue
            result = re.match(r'FC_DECLARE_DERIVED_EXCEPTION\(\s+(\w+),\s+(\w+),\s+(\d+),\s+\"([\w .-]+)\"\s?\)', line)
            if result:
                exs.append((result.group(1), result.group(2), result.group(3), result.group(4)))

    with open(output, 'w') as writer:
        for ex in exs:
            writer.write('{},{},"{}"\n'.format(ex[0], ex[2], ex[3]))


if __name__ == '__main__':
    gen()
