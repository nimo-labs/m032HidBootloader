#!/bin/bash
rm umakefile
rm *.hex
for f in umakefile*
do
	echo "Processing $f..."
	ln -s $f umakefile
	umake clean
	umake
	make
	if [ $? -eq 0 ]; then
		echo -------------------
    	echo Build of $f passed
		echo -------------------
		rm umakefile
		cp build/*.hex .
	else
    	echo build of $f failed
		rm umakefile
		exit 1
	fi
done
echo -------------------
echo All builds completed successfully
echo -------------------