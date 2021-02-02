exttools
========

Some tools for reading ext2/ext3/ext4 filesystem images.

I use these mostly to extract files from android system images.

Note that ext4 is not fully supported, some files may not be extracted.

EXAMPLE
=======

This will export the entire filesystem to 'savedir'

    ext2rd  system.img  ./:savedir

ext2rd
======

    Usage: ext2rd [-l] [-v] <fsname> [exports...]
         -l       lists all files
         -B       open as block device
         -d       verbosely lists all inodes
         -o OFS1/OFS2  specify offset to efs/sparse image
         -b from[-until]   hexdump blocks
       #123       hexdump inode 123
       #123:path  save inode 123 to path
       ext2path   hexdump ext2fs path
       ext2path:path   save ext2fs path to path
       ext2path/:path  recursively save ext2fs dir to path
    note: ext2path must not start with a slash
    note2: /dev/rdiskNN is much faster than /dev/diskNN


Build instructions
==================

There are two ways of building `extfstools`:
 * using cmake, actually, invoked using the default Makefile.
   * type: `make`  or `make vc` for a windows build.
 * using make with Makefile.unix
   * type: `make -f Makefile.unix`, this should also work in windows.


AUTHOR
======

Willem Hengeveld <itsme@xs4all.nl>

