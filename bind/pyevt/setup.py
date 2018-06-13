import sys
from setuptools import setup
import setuptools
from setuptools import find_packages

setup(
name="pyevt",
version="0.0.1",
author="everiToken",
author_email="everiToken@gmail.com",
description="python interface of evt",
long_description=open('README.rst'),
license="MIT",
url="http://github/everitoken/evt",
packages=setuptools.find_packages(),
install_requires=[],
classifiers=(
"Programming Language::Python::3",
"Licence::OSI Approved::MIT License",
"Operating System::OS Independent"
)
)
