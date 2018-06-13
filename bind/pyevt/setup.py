import setuptools
from setuptools import setup

setup(
    name="pyevt",
    version="0.0.1",
    author="everiToken",
    author_email="help@everitoken.io",
    description="Python bind library for everiToken",
    long_description=open('README.rst'),
    license="MIT",
    url="https://github.com/everitoken/evt/tree/master/bind/pyevt",
    packages=setuptools.find_packages(),
    install_requires=[],
    classifiers=(
        "Programming Language::Python::3",
        "Licence::OSI Approved::MIT License",
        "Operating System::OS Independent"
    )
)
