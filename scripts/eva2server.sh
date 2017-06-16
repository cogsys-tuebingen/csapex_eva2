#!/bin/bash 
source /opt/ros/kinetic/setup.bash --extend
roscd csapex_eva/scripts > /dev/null
dir=`pwd`
cd - > /dev/null 2>&1
if [[ $1 ]]; then
	java -Djava.util.logging.config.file=logging.properties -jar $dir/eva2server.jar --port $1
else
	java -Djava.util.logging.config.file=logging.properties -jar $dir/eva2server.jar
fi
