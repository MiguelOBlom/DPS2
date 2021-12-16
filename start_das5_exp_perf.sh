# Miguel Blom
# In this experiment we want to measure the time it takes to add 100 transactions given different block sizes to a network of 20 peers

DPS2_DIR=/home/$(whoami)/DPS2

module load prun
module load python/3.6.0

./stop_das5.sh

#DPS_NDISTANCE=$4
#DPS_NTRANSACTIONS=$3
#DPS_TIME=$2
#DPS_NNODES=$1
DPS_NNODES=3 # 20 empty peers, a tracker and the peer with the data
DPS_TIME="00:15:00"

TRACKER_PORT=8080
WORKER_PORT=1234
PEER_PORT=1234

for BLOCK_SIZE in 100 #2 5 10 20 50 100
do
	# Generate config
	echo "// Socket family" > $DPS2_DIR/application/config.h
	echo "#define DOMAIN AF_INET" >> $DPS2_DIR/application/config.h
	echo "// CRC Polynomial" >> $DPS2_DIR/application/config.h
	echo "#define POLY_TYPE uint16_t" >> $DPS2_DIR/application/config.h
	echo "#define POLY 0b1011101011010101" >> $DPS2_DIR/application/config.h
	echo "// Peer configuration" >> $DPS2_DIR/application/config.h
	echo "#define HEARTBEAT_PERIOD 5" >> $DPS2_DIR/application/config.h
	echo "// Tracker configuration" >> $DPS2_DIR/application/config.h
	echo "#define TIMEOUT_THRESHOLD 60" >> $DPS2_DIR/application/config.h
	echo "#define TRACKER_QUEUE_SIZE 20" >> $DPS2_DIR/application/config.h
	echo "// Application configuration" >> $DPS2_DIR/application/config.h
	echo "#define DIFFICULTY 24" >> $DPS2_DIR/application/config.h
	echo "#define MAX_TRANSACTIONS $BLOCK_SIZE" >> $DPS2_DIR/application/config.h
	echo "#define ID_TYPE char" >> $DPS2_DIR/application/config.h
	echo "// Chance data gets mutated (on receive)" >> $DPS2_DIR/application/config.h
	echo "#define BITFLIP_CHANCE 0" >> $DPS2_DIR/application/config.h

	# Build the project
	(cd application; make clean; make)

	for REP in 1 #{1..20}
	do
		
		# Generate 100 transactions for our single peer (we need 2 peers, a sender and a receiver, but we can join these files)
		echo "Generating transactions..."
		EXPERIMENT_NAME=$(python3 $DPS2_DIR/application/transaction_generator/transaction_generator.py 2 100)
		cat $DPS2_DIR/$EXPERIMENT_NAME/0.trc $DPS2_DIR/$EXPERIMENT_NAME/1.trc > $DPS2_DIR/$EXPERIMENT_NAME/data.trc

		# Reserve nodes and get their names
		echo "Reserving nodes..."
		NODES=$(./reserve_nodes.sh $DPS_NNODES $DPS_TIME)

		# Seperate the tracker from the workers
		TRACKER=$(echo $NODES | awk '{print $1}')
		NODES=$(echo $NODES | awk '{$1=""; print $0}')
		SEP_WORKER=$(echo $NODES | awk '{print $1}')
		WORKERS=$(echo $NODES | awk '{$1=""; print $0}')

		echo "Experiment $EXPERIMENT_NAME"

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
		for WORKER in $WORKERS;
		do
			echo "Starting worker $WORKER..."
			WORKER_IP=$(ssh $WORKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')
			touch $DPS2_DIR/$EXPERIMENT_NAME/${WORKER_IP}_${PEER_PORT}.trc # Empty trace
			screen -d -m -S $WORKER ssh -t $WORKER 'exec bash -l < DPS2/run_peer_init.sh'
			
			# Make sure the worker is ready
			while [ ! -f $DPS2_DIR/$EXPERIMENT_NAME/out/${WORKER_IP}_${PEER_PORT}.out ]
			do
			  sleep 1
			done
		done

		# Add worker seperately
		echo "Starting seperate worker $SEP_WORKER..."
		WORKER_IP=$(ssh $SEP_WORKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')
		mv $DPS2_DIR/$EXPERIMENT_NAME/data.trc $DPS2_DIR/$EXPERIMENT_NAME/${WORKER_IP}_${PEER_PORT}.trc
		screen -d -m -S $SEP_WORKER ssh -t $SEP_WORKER 'exec bash -l < DPS2/run_peer.sh'

		while [ ! -f $DPS2_DIR/$EXPERIMENT_NAME/out/${WORKER_IP}_${PEER_PORT}.out ]
		do
		  sleep 1
		done

		# Change the experiment name
		./stop_das5.sh
		mv $DPS2_DIR/$EXPERIMENT_NAME $DPS2_DIR/exp1_blocksize_${BLOCK_SIZE}_rep_${REP}

	done
done





