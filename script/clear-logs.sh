#!/usr/bin/env bash
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
mkdir -p $DIR/../logs
rm -f $DIR/../logs/*
