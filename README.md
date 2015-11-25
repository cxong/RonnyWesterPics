Ronny Wester Pics
===============

Ronny Wester wrote a few DOS games in the 90s - C-Dogs, Cyberdogs, Magus. These games used a simple paletted image format in a single binary file.

This utility decodes these graphics files (.px extension in C-Dogs and Cyberdogs) and outputs individual bitmap files. Note that some colours may not work because the colours themselves are palette indices that will be swapped in game.

Utility uses [libbmp](https://code.google.com/p/libbmp/) (LGPL) to produce bitmap files.

## To use
Simply drag a graphics file over the executable `ronnypix`, or run:

    ronnypix graphicsfile.px

Bitmaps will be created in the current directory.

Known games and files that work:

**C-Dogs**

-  cdogs.px
-  cdogs2.px
-  font.px

**Cyberdogs**

- dogs.px

**Magus**

- magus.art
- world.mgs (actually just a tileset file, no graphics data available, requires magus.art)
