# Veretino
### The app allows to verify folder-wide data integrity, for example, after numerous transfers and recopies, uploads/downloads from clouds and storages, transfer to other devices, and so on...

Veretino calculates checksums for each file in the specified folder and saves this list for later checks. The resulting database (local json file containing file paths and their checksums) can be used to check the integrity of individual files, as well as to check the entire folder contents for data changes.
Veretino can also determine if there are new or missing files in the given directory relative to the list.
Filters also can be applied to exclude unnecessary files.

The App is Qt based and cross platform.

<p align="center">
  <img src="screenshots/veretino_mainview.png">
</p>

<p align="center">
after computing checksums the Veretino app creates a json database like this:
</p>
<p align="center">
<img src="screenshots/jsondb_example.png">
</p>

<p align="center">
Some settings available
</p>

<p align="center">
  <img src="screenshots/veretino_settings.png">
</p>

<p align="center">
by parsing the database (a regular Json file, by the way), you can check the entire list of files or selected ones for matching SHA checksums, as well as check and update the presence / absence of files on the disk
</p>

<p align="center">
  <img src="screenshots/veretino_newlost.png">
</p>

The App allows to avoid unexpected data loss*, for example, in the event of a disk error or incomplete download. Checking across the entire folder and multiple subfolders allows to find data loss in any of the contained files.

#### *Please note that the Veretino app does not repair the data and is not able to prevent the data loss, but only serves to data integrity verification
