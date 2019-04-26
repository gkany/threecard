#!/bin/bash

#Usage: ./make.sh file

file=$1

eosio-cpp -abigen -I. -o  ${file}.wasm  ${file}.cpp

