#/usr/bin/bash

opennidir="$1"
installdir="lib/macosx/OpenNI2"

rm -rv $installdir
mkdir $installdir
cp -rv "$opennidir/Bin/x64-Release/"* "$installdir"
cp -rv "$opennidir/ThirdParty/PSCommon/XnLib/Bin/x64-Release/"* "$installdir"


