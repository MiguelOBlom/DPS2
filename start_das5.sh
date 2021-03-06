# Author: Miguel Blom
# Call this script with ./start_das5.sh DPS_NNODES DPS_TIME DPS_NTRANSACTIONS DPS_NDISTANCE
# e.g.                  ./start_das5.sh 10 00:10:00 100 30
# To process 100 transactions using 10 nodes which are allocated for 00:10:00 and 30 seconds apart


DPS2_DIR=/home/$(whoami)/DPS2

module load prun
module load python/3.6.0

./stop_das5.sh

DPS_NDISTANCE=$4
DPS_NTRANSACTIONS=$3
DPS_TIME=$2
DPS_NNODES=$1


TRACKER_PORT=8080
WORKER_PORT=1234
PEER_PORT=1234

echo "Generating transactions..."
EXPERIMENT_NAME=$(python3 $DPS2_DIR/application/transaction_generator/transaction_generator.py $((DPS_NNODES-1)) $DPS_NTRANSACTIONS)
echo "Experiment $EXPERIMENT_NAME"

# Reserve nodes and get their names
echo "Reserving nodes..."
NODES=$(./reserve_nodes.sh $DPS_NNODES $DPS_TIME)

# Seperate the tracker from the workers
TRACKER=$(echo $NODES | awk '{print $1}')
WORKERS=$(echo $NODES | awk '{$1=""; print $0}')

# Get the tracker IP
TRACKER_IP=$(ssh $TRACKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')

echo "Setting up network information..."
echo export TRACKER_IP=$TRACKER_IP > $DPS2_DIR/netinfo.txt
echo export TRACKER_PORT=$TRACKER_PORT >> $DPS2_DIR/netinfo.txt
echo export PEER_PORT=$PEER_PORT >> $DPS2_DIR/netinfo.txt
echo export EXPERIMENT_NAME=$EXPERIMENT_NAME >> $DPS2_DIR/netinfo.txt

# Start up the tracker
echo "Starting tracker..."
screen -d -m -S tracker ssh -t $TRACKER 'exec bash -l < DPS2/run_tracker.sh'

echo "Starting peers..."
i=0
for WORKER in $WORKERS;
do
	echo "Starting worker $WORKER..."
	WORKER_IP=$(ssh $WORKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')
	mv $DPS2_DIR/$EXPERIMENT_NAME/$i.trc $DPS2_DIR/$EXPERIMENT_NAME/${WORKER_IP}_${PEER_PORT}.trc
	screen -d -m -S $WORKER ssh -t $WORKER 'exec bash -l < DPS2/run_peer_init.sh'
	
	# We can use the following to check whether a peer has handled all its transactions
	#while [ ! -f $DPS2_DIR/$EXPERIMENT_NAME/out/${WORKER_IP}_${PEER_PORT}.out ]
	#do
	#  sleep 1
	#done

	i=$((i+1))
	echo "Waiting $DPS_NDISTANCE seconds..."
	sleep $DPS_NDISTANCE
done

screen -ls

