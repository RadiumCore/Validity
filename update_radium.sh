git pull
BITCOIN_ROOT=$(pwd) 
BDB_PREFIX="${BITCOIN_ROOT}/build"
./autogen.sh
./configure LDFLAGS="-L${BDB_PREFIX}/lib/" CPPFLAGS="-I${BDB_PREFIX}/include/" --without-gui
make
cd src
chmod 775 radium13d
cp radium13d /usr/bin
echo "update complete"

