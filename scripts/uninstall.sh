#! /bin/bash

# Removes files installed by <make_install.sh> script from the system.

remove_file() {
  if [ -e "$1" ]; then
    sudo rm -r -- "$1"
    echo "$1 removed."
  else
    echo "$1 does not exist."
  fi
}

remove_file /usr/share/applications/veretino.desktop
remove_file /usr/share/icons/hicolor/scalable/apps/veretino.svg
#remove_file /usr/share/pixmaps/veretino.png
remove_file /usr/bin/veretino

echo "All done..."
exit
