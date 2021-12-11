# Miguel Blom

export LD_LIBRARY_PATH=/home/$(whoami)/DPS2/p2p/sqlite3:$LD_LIBRARY_PATH
source /home/$(whoami)/DPS2/trackerinfo.txt

IP=$(ifconfig | grep inet | grep -o '10\.149\.\S*' | awk -F . '$NF !~ /^255/')
PORT=1234

echo /home/$(whoami)/DPS2/p2p/peer $TRACKER_IP $TRACKER_PORT $IP $PORT
/home/$(whoami)/DPS2/p2p/peer $TRACKER_IP $TRACKER_PORT $IP $PORT
sleep infinity
