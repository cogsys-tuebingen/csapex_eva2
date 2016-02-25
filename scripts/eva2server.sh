#!/bin/bash
if [[ $1 ]]; then
	java -Djava.util.logging.config.file=logging.properties -jar eva2server.jar --port $1
else
	java -Djava.util.logging.config.file=logging.properties -jar eva2server.jar
fi
