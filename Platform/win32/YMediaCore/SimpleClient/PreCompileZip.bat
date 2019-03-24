@echo off
echo StartZipFile
pushd Res
del MainWindowZip.zip
7z a -tzip MainWindowZip.zip "MainWindowRes\*" -r
7z a -tzip MainWindowZip.zip "MainWindow.xml" -r
echo EndZipFile
pause