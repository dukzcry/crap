#!/bin/sh

# $dukzcry$
# For as early Tiger as 10.4.1. You'll need more fresh ruby (install it, for example, via pkgsrc)

fetch_cmd="curl -fksSL"

if [ "$1" == "install" ]; then
	ruby -e "$($fetch_cmd https://raw.github.com/sceaga/dead-homebrew/tiger/Library/Contributions/install_homebrew.rb | sed -e 's/-o pipefail//' -e 's#mxcl/homebrew/tarball/master#sceaga/dead-homebrew/tarball/tiger#' -e 's/10.5/10.4/')"
fi

if [ "$1" == "fix" ]; then
	cd /usr/local/Library/Homebrew
	for i in `grep -Ilr "mxcl/master" .`
		do cp ${i} ${i}.orig; sed 's#mxcl/master#sceaga/tiger#' ${i}.orig > ${i}
	done
	# compat
	cp cleaner.rb cleaner.rb.orig
	sed 's/file -h/file/' cleaner.rb.orig > cleaner.rb
	cd ../../bin
	cp brew brew.orig
	sed -e 's#\(require? \)which_s \(.*\)#\1`find /usr/local/Library/Contributions/cmds -name \2`#' \
	    -e 's#which_s \(.*\)#require? `find /usr/local/Library/Contributions/cmds -name \1`#' \
	brew.orig > brew
	#
	# compiler fixes
	cd ../Library/Homebrew/extend
	cp ENV.rb ENV.rb.orig
	# gcc 4.2 -> 4.0
	sed -e 's/def gcc_4_0_1/def 2gcc/' -e 's/alias_method :gcc_4_0, :gcc_4_0_1/alias_method :gcc_4_2, :2gcc/' \
	    -e 's/def gcc/def gcc_4_0_1/' -e 's/alias_method :gcc_4_2, :gcc/alias_method :gcc_4_0, :gcc_4_0_1/' \
	    -e 's/2gcc/gcc/' \
	    -e 's/nocona/pentium-m/' -e 's/ssse3/sse2/' ENV.rb.orig > ENV.rb # cpu opts: -march=pentium-m, -msse2, adjust for your needs
	#
fi
