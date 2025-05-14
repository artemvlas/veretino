# pathstr
### _A small library for handling filesystem paths as strings._
The library performs general operations with the file system paths, processing them as strings.
More suitable for processing a large number of strings/paths than stock functions (QFile/QFileInfo/QDir).

### The project purpose:
Creation of a minimalist and publicly accessible library, which is superior in speed and capabilities of the default Qt tools.

### Key features:
* Joining paths. Automatic check of the separator presence.
* Getting a relative path.
* Changing the file extension.
* Obtaining a parent folder. Even if the path ends with the slash.
* Checking a match of the file extension with the listed ones.
