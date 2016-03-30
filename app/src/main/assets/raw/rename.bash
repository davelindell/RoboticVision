#!/bin/bash
i=1;
for f in *.jpg; do
mv "$f" "$( printf "%02d.jpg" $i )"; ((i++)); 
done
