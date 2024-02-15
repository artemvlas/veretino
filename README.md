# Veretino: folder data audit and verification
### The app allows to verify folder-wide data integrity, for example, after numerous transfers and recopies, uploads/downloads from clouds and storages, transfer to other devices, and so on...

#### Key features:
* Folder/filelist audit: analysis and verification of folder contents, looking for folder-wide data changes.
* Multiple checksums calculator.
* Finding damaged files in a folder.
* Checksums calculation, store and verification for individual files.
* Comparing files by checksum.

### Veretino can check the integrity of a large amount of data, for example, after a disk or file system error occurs, bad or unreadable sectors appear, a sudden power outage and similar cases...

#### How it works:
* To begin with, the program calculates a list of checksums for all files (or for certain types of files) in the specified folder (and its subfolders) and stores it in the local database.
* Once the list is ready, you can check the integrity of the data in this folder by comparing their checksums with those calculated and saved earlier. This can also be done only with selected file types by applying filters.
* Also, the saved list (a database of folder contents) makes it possible to find new or missing files in a folder by comparing the current contents with a previously saved list.
* The program allows to analyze the contents of folders by file types, their number and size. Finding the largest and most numerous file types, which is useful when creating a database with a specific filter.
* And when working with individual files, you can calculate the checksum and save it in the summary (*.sha1/256/512). And also check the integrity of the file using both the existing summary and the checksum from the clipboard.

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
