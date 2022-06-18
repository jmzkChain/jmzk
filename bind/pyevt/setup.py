from setuptools import setup, find_packages

setup(
    name="pyjmzk",
    version="0.3",
    author="jmzkChain",
    author_email="help@jmzkChain.io",
    description="Python bind library for jmzkChain",
    long_description=open("README.rst").read(),
    license="MIT",
    url="https://github.com/jmzkChain/jmzk/tree/master/bind/pyjmzk",
    packages=find_packages(),
    install_requires=['cffi'],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS :: MacOS X"
    ],
)
