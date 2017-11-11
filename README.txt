For this lab I added optional challenge #2, support for large files.
To do this, for file uploads I recursively created nested metadata files, and stored
the total depth of recursion. I pass this information on to the potential downloader.
The node downloading the file will know how many nested metafiles there are, and as a result,
can build up each metadata file upon receiving the pieces in Block Replies. This is all done in 
the main.cc file.

Issues:
My biggest issue here was collions in the QVariantMap "fileHash" which maps the sha1 hash value of a block
to its data values. The collisions cause data overwrite and were incredibly hard to detect. In an attempt
to fix this I added support for a QVariantList hash value such that if collision occurs, multiple values
can be stored and the sender can loop through these values until the desired one is found.