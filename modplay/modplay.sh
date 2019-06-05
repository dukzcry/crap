#!/usr/bin/env nix-shell
#!nix-shell -i sh -p coreutils curl dtrx uade123 openmpt123
#xmp

[ "$1" == "" ] && echo "usage: $0 <artist> <file>..." && exit

TMPDIR=/tmp/modplay

domain[0]="post.audioscrobbler.com"
user[0]="dukzcry"
password[0]=""
domain[1]="turtle.libre.fm"
user[1]="dukzcry"
password[1]=""
domain[2]="proxy.listenbrainz.org"
user[2]="dukzcry"
password[2]=""

auth() {
        domain=$1
        user=$2
        password=$3

        timestamp=`date +%s`
        auth=$(echo -n `echo -n $password|md5sum|cut -d ' ' -f 1`$timestamp|md5sum|cut -d ' ' -f 1)
        curl -s "http://$domain/?hs=true&p=1.2.1&c=xmp&v=0.1&u=$user&t=$timestamp&a=$auth"
}
scrobble() {
        read -d '' status sessionID npurl surl <<< $1
        [ "$status" == "OK" ] && curl -s $surl \
                --data "s=$sessionID" \
                --data-urlencode "a[0]=$2" \
                --data-urlencode "t[0]=$3" \
                --data "i[0]=$((`date +%s` - $4))&o[0]=P&r[0]=&l[0]=$4&b[0]=&n[0]=&m[0]="
}
# we need to stay interactive, no "find" forking
play() {
for i in *; do
if [ ! -d "$i" ]; then
        start=`date +%s`
        uade123 "$i"
        time=$((`date +%s` - $start))
        # uade123 doesn't return error code
        if [ $time -le 5 ]; then
                start=`date +%s`
                #xmp "$i"
                openmpt123 "$i"
                time=$((`date +%s` - $start))
        fi
        if [ $time -ge 30 ]; then
                for j in ${!sessions[@]}; do
                        scrobble "${sessions[$j]}" "$artist" "$i" $time
                done
        fi
else
        (cd "$i" && play)
fi
done
}

artist=$1; shift
curdir=`pwd`
sessions=()
for i in ${!domain[@]}; do
        session=`auth ${domain[$i]} ${user[$i]} ${password[$i]}`
        sessions[$i]=$session
done

mkdir -p $TMPDIR 2>/dev/null
cd `mktemp -d`
while [ "$1" != "" ]; do
        path=`cd "$curdir" && readlink -f "$1"`
        dtrx -qqnr "$path" || ln -s "$path" . 2>/dev/null
        shift
done

play
