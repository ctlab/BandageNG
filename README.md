[![License](https://img.shields.io/badge/licence-GPLv3-blue)](https://www.gnu.org/licenses/gpl-3.0)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/ctlab/BandageNG?include_prereleases)
[![GitHub Downloads](https://img.shields.io/github/downloads/ctlab/BandageNG/total.svg?style=social&logo=github&label=Download)](https://github.com/ctlab/BandageNG/releases)
![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/ctlab/BandageNG/test.yml?branch=dev)

# <img src="http://rrwick.github.io/Bandage/images/logo.png" alt="Bandage" width="115" height="115" align="middle">Bandage-NG

## IMPORTANT: This is a fork of original Bandage repo. It may contain new features, new bugs and otherwise be not compatible with the Bandage by Ryan Wick. The documentation, instructions, etc. might be incomplete, missed and inaccurate as well

## Table of Contents
* [Intro](https://github.com/asl/BandageNG#intro)
* [Help](https://github.com/asl/BandageNG#help)
* [Pre-built binaries](https://github.com/asl/BandageNG#pre-built-binaries)
* [Building from source](https://github.com/asl/BandageNG#building-from-source)
* [Contributing](https://github.com/asl/BandageNG#contributing)
* [Citation](https://github.com/asl/BandageNG#citation)
* [Credits](https://github.com/asl/BandageNG#credits)
* [License](https://github.com/asl/BandageNG#license)


## Intro

Bandage-NG is a GUI program that allows users to interact with the assembly graphs made by *de novo* assemblers such as <a href="http://cab.spbu.ru/software/spades" target="_blank">SPAdes</a>, <a href="https://github.com/voutcn/megahit" target="_blank">MEGAHIT</a> and others.

*De novo* assembly graphs contain not only assembled contigs but also the connections between those contigs, which were previously not easily accessible. Bandage visualises assembly graphs, with connections, using graph layout algorithms. Nodes in the drawn graph, which represent contigs, can be automatically labelled with their ID, length or depth. Users can interact with the graph by moving, labelling and colouring nodes. Sequence information can also be extracted directly from the graph viewer. By displaying connections between contigs, Bandage opens up new possibilities for analysing and improving *de novo* assemblies that are not possible by looking at contigs alone.

## Help

Bandage-NG documentation is available on the <a href="https://github.com/ctlab/BandageNG/wiki" target="_blank">Bandage-NG GitHub wiki</a>.

Bandage-NG help tips are also built into the program. Throughout the UI, you will find these icons next to controls and settings: <img src="http://rrwick.github.io/Bandage/images/helptext.png" alt="help text icon" width="16" height="16">. Click them to see a description of that element of Bandage.

## Prerequisites (for building from the source code)
  * Qt 6
  * CMake
  * C++17-compliant compiler

## Pre-built binaries

Pre-built Linux and Mac binaries are available from [Releases](https://github.com/ctlab/BandageNG/releases) page.

## Building from source

### CMake
```shell
mkdir build
cd build
cmake ..
make
```

## Contributing

New contributors are welcome! If you're interested or have ideas, please use Issues section in the repo.


## Citation

TBD


## Credits

Bandage-NG makes use of the <a href="http://www.ogdf.net/" target="_blank">OGDF</a> library for performing graph layout algorithms. Big thanks goes out to the OGDF developers for their excellent work!

* <a href="https://github.com/rrwick" target="_blank">Ryan Wick</a> (author of original Bandage)
* <a href="https://github.com/rchikhi" target="_blank">Rayan Chikhi</a>
* <a href="https://github.com/epruesse" target="_blank">Elmar Pruesse</a>
* <a href="https://github.com/wafemand" target="_blank">Andrey Zakharov</a>
* <a href="https://github.com/asl" target="_blank">Anton Korobeynikov</a>

## License

GNU General Public License, version 3
