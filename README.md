# Veretino: folder-wide data audit and integrity verification
### The app allows to verify folder-wide data integrity, for example, after numerous transfers and recopies, uploads/downloads from clouds and storages, transfer to other devices, and so on...

#### Key features:
* Multiple checksums calculator.
* Analysis and verification of folder contents.
* Calculation a list of checksums for all files in the specified folder and store it in a local database. Verification the integrity of all files in a certain folder by comparing their checksums with those calculated and stored earlier in the database. This can also be done with only selected file types by applying filters.
* Calculation a checksum of the specified file and store it in a summary (*.sha1/256/512). Verification the integrity of the file against a previously saved summary (or checksum from the clipboard).
* Finding damaged files in a folder.
* Looking for new or missing files in the folder by comparing the current contents with the previously saved list.
* Comparing files by checksum.
* Analysis of folder contents by file types, their number and size. Finding the largest and most numerous file types, which is convenient when creating a database with a specific filter.

### Veretino can check the integrity of a large amount of data, for example, after a disk or file system error occurs, bad or unreadable sectors appear, a sudden power outage and similar cases...
<p align="center">
  <br><em>Veretino calculates checksums for each file in the specified folder and saves this list for later checks.</em>
  <br><img src="screenshots/veretino_mainview.png">
  <br><em>Filters also can be applied to exclude unnecessary files.</em>
  <br>
  <br><em>The resulting database is a local json file containing a list of paths and checksums.
  <br>It can be used to check the integrity of individual files, as well as to check the entire folder contents for data changes.</em>
  <br><img src="screenshots/jsondb_example.png">
</p>

<p align="center">
  <br><em>Opening the previously created database, you can check the entire list of files or selected ones for matching the checksums</em>
  <br><img src="screenshots/veretino_newlost.png">
  <br><em>Veretino can also determine if there are new or missing files in the given directory relative to the list.
  <br>This information can be easily updated if needed.</em>
</p>

The App allows to avoid unexpected data loss*, for example, in the event of a disk error or incomplete download. Checking across the entire folder and multiple subfolders allows to find data loss in any of the contained files.

#### *Please note that the Veretino app does not repair the data and is not able to prevent the data loss, but only serves to data integrity verification
---
Veretino App is Qt based and cross platform. Prebuilt packages for Linux and Windows are available [here](https://github.com/artemvlas/veretino/releases)
For users of Arch-based distros, Veretino also is available on the [AUR](https://aur.archlinux.org/packages/veretino)  

#### Building the app is very easy:
* Download and extract the source code, or type in terminal:

      git clone https://github.com/artemvlas/veretino
* execute 'makeScripts/makeInstall.sh'
* Or do it yourself:

      mkdir build
      cd build
      qmake ..
      make -j$(nproc)
      sudo make install
