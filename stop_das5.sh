# Miguel Blom

preserve -c $(preserve -llist | tail -n+4 | grep $(whoami) | sort -r | head -n1 | awk '{print $1}')
