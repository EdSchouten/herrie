#!/bin/sh

if [ $# -ne 1 -a $# -ne 2  ]
then
	echo "usage: $0 version [version]"
	exit 1;
fi

VER1="tags/herrie-$1"

if [ $# -eq 2 ]
then
	VER2="tags/herrie-$2"
else
	VER2="trunk/herrie"
fi
	
svn diff "http://svn.il.fontys.nl/herrie/$VER1" \
	"http://svn.il.fontys.nl/herrie/$VER2"
