#!/usr/bin/env bash
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
$DIR/clear-logs.sh

TARGET_NAME=dhserver
PROJ_DIR=$DIR/..
BIN_DIR=$PROJ_DIR/bin
LOG_DIR=$PROJ_DIR/logs

$BIN_DIR/dhserver 0 9000 0     > $LOG_DIR/log0 2>&1 &
$BIN_DIR/dhserver 1 9001 5000  > $LOG_DIR/log1 2>&1 &
$BIN_DIR/dhserver 2 9002 10000 > $LOG_DIR/log2 2>&1 &
$BIN_DIR/dhserver 3 9003 15000 > $LOG_DIR/log3 2>&1 &
$BIN_DIR/dhserver 4 9004 20000 > $LOG_DIR/log4 2>&1 &
$BIN_DIR/dhserver 5 9005 25000 > $LOG_DIR/log5 2>&1 &
$BIN_DIR/dhserver 6 9006 30000 > $LOG_DIR/log6 2>&1 &
$BIN_DIR/dhserver 7 9007 35000 > $LOG_DIR/log7 2>&1 &
$BIN_DIR/dhserver 8 9008 40000 > $LOG_DIR/log8 2>&1 &
$BIN_DIR/dhserver 9 9009 45000 > $LOG_DIR/log9 2>&1 &