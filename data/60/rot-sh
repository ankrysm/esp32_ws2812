F1=@h1
F2=@h2
FINIT=@init

curl  192.168.2.228/api/v1/file -d $FINIT
for l in {0..99}
do
 curl -s 192.168.2.228/api/v1/file -d $F1 >>/dev/null
 curl -s 192.168.2.228/api/v1/file -d $F2 >>/dev/null
done
