#!/bin/bash
rm umakefile
for f in umakefile*
do
	echo "Processing $f..."
	ln -s $f umakefile
	umake clean
	../umake
	make
	if [ $? -eq 0 ]; then
    	echo Build of $f passed
		rm umakefile
	else
    	echo build of $f failed
		rm umakefile
		exit 1
	fi
done