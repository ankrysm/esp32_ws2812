./f.sh init
./f.sh h1
while true
do
	echo links
	for i in {1..60}; do ./rot.sh 1 >/dev/null; done 
	echo rechts
	for i in {1..60}; do ./rot.sh -1 >/dev/null; done
done