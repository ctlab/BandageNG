[![License](https://img.shields.io/badge/licence-GPLv3-blue)](https://www.gnu.org/licenses/gpl-3.0)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/ctlab/BandageNG?include_prereleases)
[![GitHub Downloads](https://img.shields.io/github/downloads/ctlab/BandageNG/total.svg?style=social&logo=github&label=Download)](https://github.com/ctlab/BandageNG/releases)
![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/ctlab/BandageNG/test.yml?branch=dev)

# <img src="http://rrwick.github.io/Bandage/images/logo.png" alt="Bandage" width="115" height="115" align="middle">Bandage-NG

IMPORTANT: This is a fork of <a href="https://github.com/asl/BandageNG">Bandage-NG repo</a>. It may contain new features, new bugs and otherwise be not compatible with the Bandage by Ryan Wick.

## Table of Contents
* [Intro](https://github.com/ctlab/BandageNG#intro)
* [Pre-built binaries](https://github.com/ctlab/BandageNG/wiki#pre-built-binaries)
* [Building from source](https://github.com/ctlab/BandageNG/wiki#prerequisites-for-building-from-the-source-code)


## Intro
Bandage-NG documentation is available on the <a href="https://github.com/ctlab/BandageNG/wiki" target="_blank">Bandage-NG GitHub wiki</a>.

New version of BandageNG contains new features:
1. [Rotate contig](https://github.com/ctlab/BandageNG/wiki#contigs-rotation) - the ability for nodes' rotation.
2. [Hi-C links](https://github.com/ctlab/BandageNG/wiki#hi-c-links-visualization) - this functionality allows to visualize Hi-C links between different contigs on the de Bruijn graph. Hi-C links are drawn as dotted lines connecting the midpoints of contigs.
3. [ML model](https://github.com/ctlab/BandageNG/wiki#predictive-model-visualization) - this functionality allows to visualize RandomForest, AdaBoost or Gradient Boosted Decision Trees machine learning. Also, the implementation of BandageNG supports mapping features, used in predictive model, on the nodes (contigs) in de Bruijn graph.
4. [Multigraph](https://github.com/ctlab/BandageNG/wiki#multigraph-mode) - this functionality allows to visualize multiple graphs from different files on one screen.
5. [CSV data](https://github.com/ctlab/BandageNG/wiki#csv-data) - this functionality allows to visualize CSV data for multiple graphs. To visualize taxonomy you can use CSV data with columns that contains different level of taxonomy (Superkingdom», Phylum, Class, Order, Family, Genus, Species, Serotype and Strains).

## Pre-built binaries
Pre-built Linux and Mac binaries are available from [Releases](https://github.com/ctlab/BandageNG/releases) page.

**Prerequisites:**
* libfuse2

## Prerequisites (for building from the source code)
* Qt 6
* CMake
* C++17-compliant compiler

## Building from source
```shell
mkdir build
cd build
cmake ..
make
```
