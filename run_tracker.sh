export LD_LIBRARY_PATH=/home/$(whoami)/DPS2/p2p/sqlite3:$LD_LIBRARY_PATH
DB=/home/$(whoami)/DPS2/p2p.db
IP=$(ifconfig | grep inet | grep -o '10\.149\.\S*' | awk -F . '$NF !~ /^255/')
PORT=1234
echo rm -f $DB
rm -f $DB

echo /home/$(whoami)/DPS2/p2p/tracker $IP $PORT $DB
/home/$(whoami)/DPS2/p2p/tracker $IP $PORT $DB
