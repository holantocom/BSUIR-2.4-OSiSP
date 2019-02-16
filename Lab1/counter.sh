#!/bin/bash
for i in $( find "$1" -type d ); do
   count=0
   dsize=0
   for j in $( find $i -maxdepth 1 -type f ); do
       let count=count+1
       let dsize=dsize+$( stat -c%s $j )
    done
    echo "$i $dsize $count" >> $2
done
cat $2
