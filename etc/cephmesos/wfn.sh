#!/bin/bash
#parse command line
nic_names=""
original_entry=""

for i in "$@"
do
case $i in
    -n=*)
    nic_names="${i#*=}"
    shift # past argument=value
    ;;
    -e=*)
    original_entry="${i#*=}"
    shift # past argument=value
    ;;
esac
done

echo "nic_names=${nic_names}, original_entry=${original_entry}"

#wait for all NIC up and has it's IP
IFS="," read -ra NICs <<< "${nic_names}"
for i in "${NICs[@]}"; do
    echo "Waiting for $i up and get IP.."
    while ! ip addr show dev $i | grep inet 2>dev/null
    do sleep 1
    done
    echo "$i is up.."
done
#run the original entry point
sleep 5
exec $original_entry

