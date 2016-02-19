# bigBedSearch

The [bigBed file format][bb], a binary format for describing range features on sequences that is optimized for access over a network, was updated after its [initial publication][bioinf] to include extra B-Plus Tree indices in the very last section of the file. These indices allow an application to search for features by the text content of various columns in the (uncompressed) BED format. This can enable a genome browser to have a search feature that offers possible matches by name or other feature metadata to a user query. The [UCSC Genome Browser][ucsc] offers this feature within Track Hubs.

The `bigBedNamedItems` command line utility, available in the [UCSC utility binaries directory][bbni], allows searching of these B-Plus Tree indices, but only for exact matches on one column. This project provides the command line utility `bigBedSearch`, which is equivalent to `bigBedNamedItems` in that it selectively returns BED lines from a bigBed file that matches a keyword, but it will match on **prefixes** as well as exact matches, and can search all indexed columns at once. Like `bigBedNamedItems`, it can operate efficiently on remote files only accessible over HTTP(S).

In order for these indices to be included in a bigBed file, it **must** be created with the `-extraIndex` option specified, [as described here][bbex]. Otherwise, this utility will never return results.

## Installation

`git clone` this repo, `cd` into it and then `make`. This should produce a `bigBedSearch` executable in the root directory.

If you want HTTPS to work, either make sure `/usr/include/openssl` is available, or specify the equivalent `SSL_DIR` as an environment variable.

## Usage

Run `bigBedSearch` with no arguments to see the usage statement.

## License

All files under `lib/` and `include/` are copied from the `kent/src/lib` and `kent/src/inc` directories of the [kent.git source repository][kent.git] for the UCSC Genome Browser, which are "freely available for all uses," including commercial use, according to [UCSC][ucsclicense]. They have only been modified here to disable functionality not needed for this project (grep for `TRP_EXCISION`). Be aware that many other parts of the kent.git repository are not free for commercial use.

The `*.c` files in the root directory and files in `src/` are written by Theodore Pak, with components adapted from the aforementioned "freely available for all uses" area of kent.git; all new contributions are released under the MIT license (see LICENSE).

[bb]: https://genome.ucsc.edu/goldenPath/help/bigBed.html
[bioinf]: https://bioinformatics.oxfordjournals.org/content/26/17/2204.long
[bbni]: http://hgdownload.soe.ucsc.edu/admin/exe/
[bbex]: https://genome.ucsc.edu/goldenPath/help/bigBed.html#Ex3
[ucsc]: https://genome.ucsc.edu/
[kent.git]: http://genome-source.cse.ucsc.edu/gitweb/?p=kent.git;a=tree;f=src;hb=HEAD
[ucsclicense]: https://genome.ucsc.edu/license/