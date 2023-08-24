#!/usr/bin/env python

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext


class CustomBuildExtCommand(build_ext):
    def run(self):
        import numpy
        self.include_dirs.append(numpy.get_include())
        build_ext.run(self)


def ext_module():
    result = Extension(
        'oops.odin',
        sources=['oops/odinmodule.c'],
        include_dirs=['Library'],
        library_dirs=['./Library'],
        libraries=['odin', 'm'],
    )
    return result


setup(
    name='oops',
    version='1.1.1',
    description="Python interface to the Odin C-library",
    packages=find_packages(),
    package_data={'oops': ['libfft.so', 'odinmodule.h']},
    author="Michael Olberg",
    author_email="michael.olberg@chalmers.se",
    cmdclass={'build_ext': CustomBuildExtCommand},
    ext_modules=[ext_module()],
)
