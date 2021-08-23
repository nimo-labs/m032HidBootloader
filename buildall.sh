#!/bin/bash
rm umakefile
for f in umakefile*
do
	echo "Processing $f..."
	ln -s $f umakefile
	../umake.py clean
	../umake.py
	make
	if [ $? -eq 0 ]; then
    	echo Build of $f passed
		rm umakefile
		cp build/*.hex ../
	else
    	echo build of $f failed
		rm umakefile
		exit 1
	fi
	echo -------------------
	echo All builds completed successfully
	echo -------------------
	ls -l
done