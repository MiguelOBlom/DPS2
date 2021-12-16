# Miguel Blom

DPS2_DIR=/home/$(whoami)/DPS2
source $DPS2_DIR/netinfo.txt

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH":$DPS2_DIR/application/blockchain:$DPS2_DIR/application/p2p:$DPS2_DIR/application/pow:$DPS2_DIR/application/sha256:$DPS2_DIR/application/sqlite3

DB=$DPS2_DIR/p2p.db

rm -f $DB
echo /home/$(whoami)/DPS2/p2p/tracker $TRACKER_IP $TRACKER_PORT $DB
/home/$(whoami)/DPS2/p2p/tracker $TRACKER_IP $TRACKER_PORT $DB
sleep infinity
