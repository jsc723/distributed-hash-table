#!/usr/bin/env bash
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
$DIR/clear-logs.sh

TARGET_NAME=dhserver
PROJ_DIR=$DIR/..
BIN_DIR=$PROJ_DIR/bin
LOG_DIR=$PROJ_DIR/logs

IP=192.168.40.130
BIP=192.168.40.129
BPORT=9000

$BIN_DIR/dhserver 3 $IP 9000 10000 $BIP $BPORT  > $LOG_DIR/log0 2>&1 &
$BIN_DIR/dhserver 4 $IP 9001 30000 $BIP $BPORT  > $LOG_DIR/log1 2>&1 &
$BIN_DIR/dhserver 5 $IP 9002 50000 $BIP $BPORT > $LOG_DIR/log2 2>&1 &