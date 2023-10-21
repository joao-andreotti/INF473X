for tot_len in {130..160}
do
    echo "tot_len=$tot_len"
    python3 exploit.py $tot_len > /dev/null
done