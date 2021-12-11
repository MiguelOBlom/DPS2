export LD_LIBRARY_PATH=/home/$(whoami)/DPS2/p2p/sqlite3:$LD_LIBRARY_PATH
source /home/$(whoami)/DPS2/trackerinfo.txt

DB=/home/$(whoami)/DPS2/p2p.db

rm -f $DB
echo /home/$(whoami)/DPS2/p2p/tracker $TRACKER_IP $TRACKER_PORT $DB
/home/$(whoami)/DPS2/p2p/tracker $TRACKER_IP $TRACKER_PORT $DB
sleep infinity
