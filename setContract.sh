#!/bin/bash

#Usage: ./setContract.sh account file

account=$1
file=$2
dir=`pwd`

cleos="cleos"  #local test
#cleos="cleos -u http://api-mainnet.starteos.io"  #main chain

$cleos set contract ${account} ${dir} ${file}.wasm ${file}.abi -p ${account}@active

