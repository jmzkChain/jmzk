from setuptools import find_packages, setup

setup(
    name='trafficgen',
    version='0.1',
    author='jmzkChain',
    author_email='help@jmzkChain.io',
    description='Python SDK library for jmzkChain',
    long_description=open('README.rst').read(),
    license='MIT',
    url='https://github.com/jmzkChain/jmzk/tree/master/loadtest/trafficgen',
    packages=find_packages(),
    install_requires=['pyjmzksdk', 'lz4'],
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X'
    ],
    entry_points={
        'console_scripts': [
            'trafficgen = trafficgen.generator:main',
        ]
    },
)
