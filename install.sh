#!/bin/sh
set -e
sudo cp waveform.desktop /usr/share/applications/
sudo cp waveform.gschema.xml /usr/share/glib-2.0/schemas/com.github.lukawarren.waveform.gschema.xml
sudo cp waveform.mime.xml /usr/share/mime/packages/com.github.lukawarren.waveform.xml
sudo cp build/waveform /usr/bin/

sudo update-desktop-database
sudo update-mime-database /usr/share/mime

cd /usr/share/glib-2.0/schemas/ && sudo glib-compile-schemas .
echo "Done! To remove run"
echo "sudo rm /usr/bin/waveform"
echo "sudo rm /usr/share/applications/waveform.desktop"
echo "sudo rm /usr/share/glib-2.0/schemas/com.github.lukawarren.waveform.gschema.xml"
echo "sudo rm /usr/share/mime/packages/com.github.lukawarren.waveform.xml"
echo "sudo update-desktop-database"
echo "sudo update-mime-database /usr/share/mime"
