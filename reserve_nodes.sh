# Author: Miguel Blom

module load prun
# Reserve nodes
(preserve -# $1 -t $2) &>/dev/null;

# Get their names
while : ; do
	sleep 1;
        NODES=$(preserve -llist | tail -n+4 | grep $(whoami) | sort -r | head -n1 | awk '{$1=$2=$3=$4=$5=$6=$8=""; if($7=="R"){$7="";print $0}}')
        [[ -z "$NODES" ]] || break
done

echo $NODES
