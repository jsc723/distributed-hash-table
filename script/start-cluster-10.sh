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


$BIN_DIR/dhserver 0 $IP 9000 0     > $LOG_DIR/log0 2>&1 &
$BIN_DIR/dhserver 1 $IP 9001 5000 $BIP $BPORT > $LOG_DIR/log1 2>&1 &
$BIN_DIR/dhserver 2 $IP 9002 10000 $BIP $BPORT > $LOG_DIR/log2 2>&1 &
$BIN_DIR/dhserver 3 $IP 9003 15000 $BIP $BPORT > $LOG_DIR/log3 2>&1 &
$BIN_DIR/dhserver 4 $IP 9004 20000 $BIP $BPORT > $LOG_DIR/log4 2>&1 &
$BIN_DIR/dhserver 5 $IP 9005 25000 $BIP $BPORT > $LOG_DIR/log5 2>&1 &
$BIN_DIR/dhserver 6 $IP 9006 30000 $BIP $BPORT > $LOG_DIR/log6 2>&1 &
$BIN_DIR/dhserver 7 $IP 9007 35000 $BIP $BPORT > $LOG_DIR/log7 2>&1 &
$BIN_DIR/dhserver 8 $IP 9008 40000 $BIP $BPORT > $LOG_DIR/log8 2>&1 &
$BIN_DIR/dhserver 9 $IP 9009 45000 $BIP $BPORT > $LOG_DIR/log9 2>&1 &