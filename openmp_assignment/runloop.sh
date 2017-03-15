#!/bin/bash

ITERS=0
while [ $ITERS -lt 3 ]; do
	let ITERS=ITERS+1;
	"$@"; 
done  
