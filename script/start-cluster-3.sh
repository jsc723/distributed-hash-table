#!/usr/bin/env bash
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
$DIR/clear-logs.sh

TARGET_NAME=dhserver
PROJ_DIR=$DIR/..
BIN_DIR=$PROJ_DIR/bin
LOG_DIR=$PROJ_DIR/logs

IP=192.168.40.129
BIP=192.168.40.129
BPORT=9000

$BIN_DIR/dhserver 0 $IP 9000 0  > $LOG_DIR/log0 2>&1 &
$BIN_DIR/dhserver 1 $IP 9001 20000 $BIP $BPORT  > $LOG_DIR/log1 2>&1 &
$BIN_DIR/dhserver 2 $IP 9002 40000 $BIP $BPORT > $LOG_DIR/log2 2>&1 &