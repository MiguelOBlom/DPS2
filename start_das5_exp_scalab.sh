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
#DPS_NNODES=22 # 20 empty peers, a tracker and the peer with the data
DPS_TIME="00:15:00"

TRACKER_PORT=8080
WORKER_PORT=1234
PEER_PORT=1234

rm -f exp_scalab.res

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
echo "#define MAX_TRANSACTIONS 5" >> $DPS2_DIR/application/config.h
echo "#define ID_TYPE char" >> $DPS2_DIR/application/config.h
echo "// Chance data gets mutated (on receive)" >> $DPS2_DIR/application/config.h
echo "#define BITFLIP_CHANCE 0" >> $DPS2_DIR/application/config.h

# Build the project
(cd application; make clean; make)


for DPS_NNODES in 4 7 12 17 22 27 32
do
	for REP in 1 {1..20}
	do		
		# Generate 100 transactions and distribute over our peers 
		echo "Generating transactions..."
		EXPERIMENT_NAME=$(python3 $DPS2_DIR/application/transaction_generator/transaction_generator.py $((DPS_NNODES - 1)) 100)
		cat $DPS2_DIR/$EXPERIMENT_NAME/*.trc > $DPS2_DIR/$EXPERIMENT_NAME/data.trc.temp
		rm $DPS2_DIR/$EXPERIMENT_NAME/*.trc
		
		for i in $(seq 0 $((DPS_NNODES - 2)))
		do 
			cp $DPS2_DIR/$EXPERIMENT_NAME/data.trc.temp $DPS2_DIR/$EXPERIMENT_NAME/$i.trc
		done
		
		rm $DPS2_DIR/$EXPERIMENT_NAME/data.trc.temp
		
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

		# Set up the rest of the network
		echo "Starting peers..."
		i=0
		for WORKER in $WORKERS;
		do
			echo "Starting worker $WORKER..."
			WORKER_IP=$(ssh $WORKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')
			mv $DPS2_DIR/$EXPERIMENT_NAME/$i.trc $DPS2_DIR/$EXPERIMENT_NAME/${WORKER_IP}_${PEER_PORT}.trc
			screen -d -m -S $WORKER ssh -t $WORKER 'exec bash -l < DPS2/run_peer_init.sh'
			
			# Make sure the worker is ready
			while [ ! -f $DPS2_DIR/$EXPERIMENT_NAME/out/${WORKER_IP}_${PEER_PORT}.out ]
			do
			  sleep 1
			done
			i=$((i+1))
		done
		
		# Add worker seperately
		echo "Starting seperate worker $SEP_WORKER..."
		start=$(date +%s.%1N)
		WORKER_IP=$(ssh $SEP_WORKER $'ifconfig | grep inet | grep -o \'10\.149\.\S*\' | awk -F . \'$NF !~ /^255/\'')
		touch $DPS2_DIR/$EXPERIMENT_NAME/${WORKER_IP}_${PEER_PORT}.trc # Empty trace
		screen -d -m -S $SEP_WORKER ssh -t $SEP_WORKER 'exec bash -l < DPS2/run_peer.sh'

		# While block 20 has not been received, the peer is not updated
		while [ ! -f $DPS2_DIR/$EXPERIMENT_NAME/out/${WORKER_IP}_${PEER_PORT}.out ]
		do
			sleep 1
		done
		
		while ! $(cat $DPS2_DIR/$EXPERIMENT_NAME/out/${WORKER_IP}_${PEER_PORT}.out | grep RECEIVED_BLOCK | grep SUCCESS | awk '{print $4}' | grep -q 20)
		do
			sleep 1
		done 
		

		end=$(date +%s.%1N)
		res=$(echo "scale=1; $end - $start" | bc)

		echo $DPS_NNODES $REP $res >> exp_scalab.res

		# Change the experiment name
		./stop_das5.sh
		rm -rf $DPS2_DIR/exp2_nnodes_${DPS_NNODES}_rep_${REP}
		mv $DPS2_DIR/$EXPERIMENT_NAME $DPS2_DIR/exp2_nnodes_${DPS_NNODES}_rep_${REP}

	done
done





