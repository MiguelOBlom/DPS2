# Author: Miguel Blom

DPS2_DIR=/home/$(whoami)/DPS2

source $DPS2_DIR/netinfo.txt

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH":$DPS2_DIR/application/blockchain:$DPS2_DIR/application/p2p:$DPS2_DIR/application/pow:$DPS2_DIR/application/sha256:$DPS2_DIR/application/sqlite3

IP=$(ifconfig | grep inet | grep -o '10\.149\.\S*' | awk -F . '$NF !~ /^255/')
echo $IP>>out.txt
mkdir -p $DPS2_DIR/$EXPERIMENT_NAME/out

echo $DPS2_DIR/application/build/application $TRACKER_IP $TRACKER_PORT $IP $PEER_PORT $DPS2_DIR/$EXPERIMENT_NAME/${IP}_${PEER_PORT}.trc $DPS2_DIR/$EXPERIMENT_NAME/out/${IP}_${PEER_PORT}.out \> $DPS2_DIR/$EXPERIMENT_NAME/${IP}_${PEER_PORT}.log >> out.txt
$DPS2_DIR/application/build/application $TRACKER_IP $TRACKER_PORT $IP $PEER_PORT $DPS2_DIR/$EXPERIMENT_NAME/${IP}_${PEER_PORT}.trc $DPS2_DIR/$EXPERIMENT_NAME/out/${IP}_${PEER_PORT}.out 1 > $DPS2_DIR/$EXPERIMENT_NAME/${IP}_${PEER_PORT}.log
sleep infinity
