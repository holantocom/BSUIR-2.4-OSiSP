#!/bin/bash

for i in $(find "$1" -type f -size "+$2c" -size "-$3c"); do
	for j in $(find "$1" -type f -size "+$2c" -size "-$3c"); do
		if [[ ! `diff -q "$i" "$j"` && ($i != $j) ]]; then
			echo "$i = $j"
		fi
	done
done

