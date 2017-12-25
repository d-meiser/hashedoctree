#!/bin/sh

if [ ! -e tbb-2018_U2 ];
then
  wget https://github.com/01org/tbb/archive/2018_U2.tar.gz
  tar xf 2018_U2.tar.gz
fi
cd tbb-2018_U2
make -j4
sh build/generate_tbbvars.sh
TBBVARS_SH=$(find -name tbbvars.sh | grep release)
source $TBBVARS_SH
cd -

