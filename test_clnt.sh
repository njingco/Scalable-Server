#!/bin/bash



for i in {1..50}
do
   ./client 192.168.1.71 100 $i &
done