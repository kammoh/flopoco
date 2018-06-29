#! /bin/sh
sed -e "s/#.*$//; s/^[[:space:]]*// ; s/[[:space:]]*$//; /^$/d" $1 > $2
