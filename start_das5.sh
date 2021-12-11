module load prun
source export_vars_giraph.sh

DPS_CMD=$3
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

# Start up the tracker
ssh $TRACKER_IP "/home/$(whoami)/DPS2/p2p/tracker $TRACKER_IP $TRACKER_PORT"

for WORKER in $WORKERS;
do
        WORKER_IP=$(ssh $WORKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')
        ssh $WORKER_IP "/home/$(whoami)/DPS2/p2p/peer $TRACKER_IP $TRACKER_PORT $WORKER_IP $WORKER_PORT"
done


# Release the nodes
preserve -c $(preserve -llist | tail -n+4 | grep $(whoami) | sort -r | head -n1 | awk '{print $1}')