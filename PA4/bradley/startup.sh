#!/bin/sh
clear 

masterPort="3010"
slavePort="3011"
availableIP="10.0.2.5"
masterOneIP="10.0.2.6"
masterTwoIP="10.0.2.7"
routerIP="10.0.2.4"

available="$availableIP:$masterPort"
masterOne="$masterOneIP:$masterPort"
masterTwo="$masterTwoIP:$masterPort"
router="$routerIP:$masterPort"

echo "Are you starting the routing server? (y/n): "
read isrouter

if [ $isrouter = "y" ]
then
    ./tsd -r router -a $available -m $masterOne -n $masterTwo -o $slavePort &
    # ./tsd -p $slavePort -r slave -o $masterPort &
else 
    # ./tsd -p $masterPort -r master -o $slavePort -s $router &
    ./tsd -p $slavePort -r slave -o $masterPort -s $router &
fi

exit 0