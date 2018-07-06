from setuptools import find_packages, setup

setup(
    name='trafficgen',
    version='0.1',
    author='everiToken',
    author_email='help@everitoken.io',
    description='Python SDK library for everiToken',
    long_description=open('README.rst').read(),
    license='MIT',
    url='https://github.com/everitoken/evt/tree/master/loadtest/trafficgen',
    packages=find_packages(),
    install_requires=['pyevtsdk'],
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X'
    ],
)
