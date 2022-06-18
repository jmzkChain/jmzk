from setuptools import find_packages, setup

setup(
    name='pyjmzksdk',
    version='0.3',
    author='jmzkChain',
    author_email='help@jmzkChain.io',
    description='Python SDK library for jmzkChain',
    long_description=open('README.rst').read(),
    license='MIT',
    url='https://github.com/jmzkChain/jmzk/tree/master/sdks/pysdk',
    packages=find_packages(),
    install_requires=['pyjmzk', 'requests'],
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X'
    ],
)
