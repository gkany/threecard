#!/bin/bash

#Usage: ./cleanContract.sh account

account=$1

cleos="cleos"  #local test
#cleos="cleos -u http://api-mainnet.starteos.io"  #main chain

$cleos set contract $account  --clear  -p ${account}@active

