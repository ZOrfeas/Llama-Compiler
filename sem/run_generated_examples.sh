#!/bin/bash

erCount=0
segCount=0

erFiles=()
segFiles=()

error="Error"
segmentation="Segmentation fault"

for i in $(seq 1 $1)
do
    if ./llamac -s < ../generated_examples/p$i.lla | grep -q "Error" 
    then 
        let "erCount+=1"; erFiles+=(p$i.lla)
    elif ./llamac -s < ../generated_examples/p$i.lla | grep -q "Segmentation fault"
    then 
        let "segCount+=1"; segFiles+=(p$i.lla)
    fi
done

failures=$(($erCount+$segCount))

echo Found $failures problems

echo 
echo $erCount errors
for s in "${erFiles[@]}"
do
    printf "$s, "
done

echo 
echo 
echo $segCount segmentation faults
for s in "${segFiles[@]}" 
do 
    printf "$s, " 
done