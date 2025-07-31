import os
from setuptools import setup
from setuptools.extension import Library
from distutils.command.build_ext import build_ext as _build_ext

BASE_DIR = os.path.dirname(os.path.realpath(__file__))


class CTypesLibrary(Library):
    pass


class build_ext(_build_ext):
    """
    Custom build_ext to:
    1. Emit .so (shared library) names for ctypes-based extensions.
    2. Suppress automatic export of PyInit_<modulename> symbols.
    """
    def build_extension(self, ext):
        self._ctypes = isinstance(ext, CTypesLibrary)
        return super().build_extension(ext)

    def get_ext_filename(self, ext_name):
        # For ctypes libraries, emit a plain .so (no Python version tag)
        if getattr(self, '_ctypes', False):
            return os.path.join(*ext_name.split('.')) + '.so'
        return super().get_ext_filename(ext_name)

    def get_export_symbols(self, ext):
        # Skip injecting PyInit_<name> for ctypes libraries
        if isinstance(ext, CTypesLibrary):
            return []
        return super().get_export_symbols(ext)


with open(os.path.join(BASE_DIR, 'README.md'), 'r', encoding='utf-8') as f:
    long_description = f.read()


setup(
    name='afaligner',
    version='0.2.0',
    url='https://github.com/r4victor/afaligner',
    author='Victor Skvortsov',
    author_email='vds003@gmail.com',
    description='A forced aligner intended for synchronization of narrated text',
    long_description=long_description,
    long_description_content_type='text/markdown',
    license='MIT',
    classifiers=[
        'Topic :: Multimedia :: Sound/Audio :: Speech',
        'Development Status :: 3 - Alpha',
        'Programming Language :: Python :: 3',
        'Programming Language :: C',
        'Operating System :: OS Independent',
        'License :: OSI Approved :: MIT License',
    ],
    keywords=['forced-alignment'],
    packages=['afaligner'],
    package_dir={'': 'src'},
    package_data={'afaligner': ['templates/*']},
    install_requires=[
        'aeneas>=1.7.3.0',
        'Jinja2>=3.1.2',
    ],
    ext_modules=[
        CTypesLibrary(
            'afaligner.c_modules.dtwbd',
            sources=['src/afaligner/c_modules/dtwbd.c']
        )
    ],
    cmdclass={'build_ext': build_ext},
)
```
