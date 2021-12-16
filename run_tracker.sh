# Miguel Blom

DPS2_DIR=/home/$(whoami)/DPS2
source $DPS2_DIR/netinfo.txt

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH":$DPS2_DIR/application/blockchain:$DPS2_DIR/application/p2p:$DPS2_DIR/application/pow:$DPS2_DIR/application/sha256:$DPS2_DIR/application/sqlite3

DB=$DPS2_DIR/p2p.db

rm -f $DB
echo $DPS2_DIR/application/build/tracker $TRACKER_IP $TRACKER_PORT $DB >> out.txt
$DPS2_DIR/application/build/tracker $TRACKER_IP $TRACKER_PORT $DB >> log.txt
sleep infinity
