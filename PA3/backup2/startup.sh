#!/bin/sh
clear 

masterPort = 3010
slavePort = 3011

available = 10.0.2.5:$masterPort
masterOne = 10.0.2.6:$masterPort
masterTwo = 10.0.2.7:$masterPort

echo "Are you starting the routing server? (y/n): "
read isrouter

if [ $isrouter = "y" ]
then
    ./tsd -r router -a $available -m $masterOne -n $masterTwo -o $slavePort
    ./tsd -p $slavePort -r slave -o $masterPort
else 
    ./tsd -p $masterPort -r master -o $slavePort
    ./tsd -p $slavePort -r slave -o $masterPort
fi

exit 0