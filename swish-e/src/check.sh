#!/bin/sh
# Simple shell to control the amount of memory used by swish-e
# while indexing
#
# It assumes that there is only one swish-e proc running
# This script is onlu for testing
#
# max allowed memory in kilobytes for swish-e
#
maxmemory=50000

while test 1
do
swishdata=`ps -o pid -o rss -o comm -ef | grep swish-e`
pid=`echo $swishdata | cut -d ' ' -f 1`
rss=`echo $swishdata | cut -d ' ' -f 2`
echo "swish-e with pid $pid is using $rss Kilobytes"

if [ $rss -gt $maxmemory ]
then
	echo "Swish is using more than $maxmemory"
	kill -9 $pid
	exit
fi
#Sleep 1 minute
sleep 60
done
