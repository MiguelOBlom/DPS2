module load prun

./stop_das5.sh

DPS_TIME=$2
DPS_NNODES=$1

TRACKER_PORT=8080
WORKER_PORT=1234

# Reserve nodes and get their names
NODES=$(./reserve_nodes.sh $DPS_NNODES $DPS_TIME)

# Seperate the tracker from the workers
TRACKER=$(echo $NODES | awk '{print $1}')
WORKERS=$(echo $NODES | awk '{$1=""; print $0}')

# Get the tracker IP
TRACKER_IP=$(ssh $TRACKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')

echo export TRACKER_IP=$TRACKER_IP > /home/$(whoami)/DPS2/trackerinfo.txt
echo export TRACKER_PORT=$TRACKER_PORT >> /home/$(whoami)/DPS2/trackerinfo.txt

# Start up the tracker
echo ssh $TRACKER_IP "rm -f p2p.db; /home/$(whoami)/DPS2/p2p/tracker $TRACKER_IP $TRACKER_PORT p2p.db"
#ssh $TRACKER_IP "/home/$(whoami)/DPS2/p2p/tracker $TRACKER_IP $TRACKER_PORT p2p.db"
#ssh -t $TRACKER_IP 'exec bash -l < DPS2/run_tracker.sh &'
screen -d -m -S tracker ssh -t $TRACKER 'exec bash -l < DPS2/run_tracker.sh'

for WORKER in $WORKERS;
do
        WORKER_IP=$(ssh $WORKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')
        #echo ssh $WORKER_IP "/home/$(whoami)/DPS2/p2p/peer $TRACKER_IP $TRACKER_PORT $WORKER_IP $WORKER_PORT"
	#ssh -t $WORKER_IP 'exec bash -l < DPS2/run_peer.sh &'
	screen -d -m -S $WORKER ssh -t $WORKER 'exec bash -l < DPS2/run_peer.sh'
	#ssh $WORKER_IP "/home/$(whoami)/DPS2/p2p/peer $TRACKER_IP $TRACKER_PORT $WORKER_IP $WORKER_PORT"
done

screen -ls


# Release the nodes
#preserve -c $(preserve -llist | tail -n+4 | grep $(whoami) | sort -r | head -n1 | awk '{print $1}')

