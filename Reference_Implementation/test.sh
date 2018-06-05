#!/bin/sh

measure () {
	valgrind --tool=massif --stacks=yes $@ &
	PID=$!
	wait $PID
	echo -n 'heap1: ' 1>&2 ; grep 'mem_heap_B'       massif.out.$PID | awk -F '=' '{ print $2 }' | sort -n | tail -1 1>&2
	echo -n 'heap2: ' 1>&2 ; grep 'mem_heap_extra_B' massif.out.$PID | awk -F '=' '{ print $2 }' | sort -n | tail -1 1>&2
	echo -n 'stack: ' 1>&2 ; grep 'mem_stacks_B'     massif.out.$PID | awk -F '=' '{ print $2 }' | sort -n | tail -1 1>&2
}

# SLOOOOOOOOOW
# measure ./rainbow-genkey pk.txt sk.txt
measure ./rainbow-sign sk.txt $0 > sig.txt
measure ./rainbow-verify pk.txt sig.txt $0

