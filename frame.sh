#!/bin/sh
ff=`ls *.ppm`
for f in $ff
do
file=`echo  ${f%.*}`
ffmpeg -i "$file".ppm "$file".jpg
done 
mkdir -p jpgs
mv *.jpg jpgs
rm *.ppm