# Veretino: a new way to organize checksums
_With this application, the integrity of both individual files and entire lists can be verified.
Analyze the contents of folders, calculate checksums, and easily find modified or corrupted data..._

### The app allows you to check the folder-wide data integrity, for example, after numerous transfers and recopies, uploads/downloads from clouds and storages, transfer to other devices, and so on...

#### Key features:
* Multiple checksums calculator.
* Folder/file-list data audit: analysis and verification of folder contents, looking for folder-wide data changes.
* Finding damaged files in a folder.
* Checksums calculation, store and verification for individual files.
* Comparing files by checksum.

### Veretino can check the integrity of a large amount of data, for example, after a disk or file system error occurs, bad or unreadable sectors appear, a sudden power outage and similar cases...

#### How it works:
* To begin with, the program calculates a list of checksums for all files (or for certain types of files) in the specified folder (and its subfolders) and stores it in the local database.
* Once the list is ready, you can check the integrity of the data in this folder by comparing their checksums with those calculated and saved earlier. This can also be done only with selected file types by applying filters.
* Also, the saved list (a database of folder contents) makes it possible to find new or missing files in a folder by comparing the current contents with a previously saved list.
* The program allows analyzing the contents of folders by file types, their number and size. Finding the largest and most numerous file types, which is useful when creating a database with a specific filter.
* And when working with individual files, you can calculate the checksum and save it in the summary (*.sha1/256/512). And also check the integrity of the file using both the existing summary and the checksum from the clipboard.

<p align="center">
  <br><em>Veretino calculates checksums for each file in the specified folder and saves this list for later checks.
  <br>Filters also can be applied to exclude unnecessary files.</em>
  <br><img src="screenshots/veretino_mainview.png">
  <br>
  <br><em>The resulting database is a local json file containing a list of paths and checksums.</em>
  <br><img src="screenshots/jsondb_example.png">
</p>

<p align="center">
  <br><em>It can be used to check the integrity of individual files, as well as to check the entire folder contents for data changes. Veretino can also determine if there are new or missing files in the given directory relative to the list.
  <br>This information can be easily updated if needed.</em>
  <br><img src="screenshots/veretino_newlost.png">
</p>

The App allows you to avoid unexpected data loss*, for example, in case of a disk error or incomplete download. Checking across the entire folder and multiple subfolders allows you to detect data loss in any of the contained files.

#### *Please note that the Veretino app does not repair the data and is not able to prevent its loss, but only serves to verify its integrity.
---
Veretino App is Qt based and cross platform. Prebuilt packages for Linux and Windows are available [here](https://github.com/artemvlas/veretino/releases)

For users of Arch-based distros: [AUR](https://aur.archlinux.org/packages/veretino)

#### Building the app is very easy:
* Download and extract the source code or type in the terminal:

      git clone https://github.com/artemvlas/veretino
* execute 'makeScripts/makeInstall.sh'
* Or do it yourself:

      mkdir build
      cd build
      qmake ..
      make -j$(nproc)
      sudo make install
