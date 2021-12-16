# Miguel Blom
echo "Cancelling previous reservations..."
RES_ID=$(preserve -llist | tail -n+4 | grep $(whoami) | sort -r | head -n1 | awk '{print $1}')
if [ -n "$RES_ID" ]
then
	preserve -c $RES_ID
fi
