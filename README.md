# bigBedSearch

The [bigBed file format][bb], a binary format for describing range features on sequences that is optimized for access over a network, was updated after its [initial publication][bioinf] to include extra B+ tree indices in the very last section of the file. These indices allow an application to search for features by the text content of various fields in the uncompressed BED data. This can enable a genome browser to search for features by name, such as the [UCSC Genome Browser][ucsc] allows within Track Hubs. The `bigBedNamedItems` command line utility [available from UCSC][bbni] also allows searching these indices, but only for exact matches and on one column at a time.

`bigBedSearch` is equivalent to `bigBedNamedItems` in that it selectively returns BED lines from a bigBed file that matches a keyword, but it will match on *prefixes* as well as exact matches, and can search all indexed columns at once (or a certain fields in a specified order). Like `bigBedNamedItems`, it can operate efficiently on remote files only accessible over HTTP(S).

In order for these indices to be present in a bigBed file, it **must** have been created with the `-extraIndex` option for `bedToBigBed` enabled, [as described here][bbex]. Otherwise, this utility will return an error.

## Installation

`git clone` this repo, `cd` into it and then `make`. This should produce a `bigBedSearch` executable in the root directory.

If you want HTTPS to work, either make sure `/usr/include/openssl` is available, or specify the equivalent `SSL_DIR` as an environment variable.

## Usage

Run `bigBedSearch` with no arguments to see the usage statement.

    $ ./bigBedSearch
    bigBedSearch - Search for items that begin with the given name in a bigBed file
    usage:
       bigBedSearch file.bb query output.bed
    options:
       -maxItems=N - if set, restrict output to first N items
       -fields=fieldList - search on this field name (OR field names, separated by commas).
            Default is to search all indexes, in the order they were saved.

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