#!/bin/sh

# Enjoy The Written Word easily without paying them
# For example, 'easy-tww.sh mc-4.6.1' will get mc and it's deps, and build them
# Tested on HP-UX
#
# -dukzcry

tmpdir=/tmp
repo=support.thewrittenword.com
src=/dists/9.1/src

path=$repo$src
db=depot-db.xml

fetch_ftp ()
{
	file=$1
	path=$2

	if [ ! -e $tmpdir/$file ]; then
		(
		echo "user anonymous localhost"
		echo "passive"
		echo "binary"
		echo "debug"
		echo "hash"
		echo "cd $path"
		echo "get $file $tmpdir/$file"
		echo "quit"
		) | /usr/bin/ftp -i -n $repo	
	fi
}

fetch_wget ()
{
	file=$1
	path=$2

	if [ ! -e $tmpdir/$file ]; then
		wget ftp://$repo$path/$file -O $tmpdir/$file
	fi
}

#for pkg in $@; do
pkg=$1
	sb=$pkg.sb
	#fetch_wget $sb $src/$pkg
	#fetch_wget $sb-db $src/$pkg
	fetch_ftp $sb $src/$pkg
	fetch_ftp $sb-db $src/$pkg 
	# limit to selected module
	dep=`sb -N $tmpdir/$sb $2 | grep install-name \
		| sed -n 's/.*install-name="\(.[^"]*\)".*/\1/p' | head -n 1`

	# searching for "..." ... not found

	# dependency "/opt/TWWfsw/..."
        if [ "$dep" = "" ]; then
                dep=`sb -N $tmpdir/$sb $2 | grep dependency \
                | sed -n 's#.*dependency "/opt/TWWfsw/\(.[^"]*\)".*#\1#p' | head -n 1`
        fi
	# error: state="install", version==5.12.2. 
        if [ "$dep" = "" ]; then
                dep=`sb -N $tmpdir/$sb $2 | grep searching \
                | sed -n 's/.*"\(.[^"]*\)".*not found/\1/p' | head -n 1`
        fi
 
	if [ "$dep" != "" ]; then
		echo 'dep:' $dep
		fetch_ftp $db $src
		#fetch_wget $db $src
			cordep=`cat $tmpdir/$sb-db | grep $dep \
				| sed -n 's/.*program="\(.[^"]*\)".*/\1/p' | head -n 1`
			if [ "$cordep" != "" ]; then
				echo 'exactmatch:' $cordep
				dep=$cordep
			else
				# clean
				dep=`echo $dep | sed -e 's/lib\(.*\)/\1/' -e 's/[0-9].*//'`
				echo 'match:' $dep
			fi
			# grep rule should be revisited
   			dodep=`cat $tmpdir/$db | grep \"$dep'[.0-9-]' \
				| sed -n 's/.*name="\(.[^"]*\)".*/\1/p'`
			echo 'rerun:' $dodep
			#$0 $dodep 
			for i in $dodep; do
				$0 $i
			done
	fi

	# regen
        dep=`sb -N $tmpdir/$sb $2 | grep install-name | head -n 1 \
                | sed -n 's/.*install-name="\(.[^"]*\)".*/\1/'`
	if [ "$dep" != "" ]; then
		$0 $pkg
	else
		[ ! -d $tmpdir/build ] && mkdir $tmpdir/build
		sb -v --builddir=$tmpdir/build --local-depot=$tmpdir $tmpdir/$sb $2
		sb -v -i --builddir=$tmpdir/build --local-depot=$tmpdir $tmpdir/$sb $2
	fi
#done
