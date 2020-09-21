filename="$1"
shift
for usrpass in "$@"
do
	echo -n $usrpass | awk -F ':' '{print $1}' | tr -d '\n' >> $filename
	echo -n ':' >> $filename
	echo -n $usrpass | base64 >> $filename
done