#!/bin/bash

MAIN_FILE=main.c
OUT_FILE=main

gcc "${MAIN_FILE}" -o "${OUT_FILE}"

stty -F /dev/tty -icanon

#./${OUT_FILE} '1' & disown
##./${OUT_FILE} '1'
#./${OUT_FILE} '2'

if [[ -n "${1}" && "${1}" = *"X"* ]]; then
  ./${OUT_FILE} '1' 'X' '2' & disown
  #./${OUT_FILE} '1'
  ./${OUT_FILE} '2' 'X' '2'
elif [[ -n "${1}" && "${1}" = *"V"* ]]; then
  ./${OUT_FILE} '1' 'V' '2' & disown
  #./${OUT_FILE} '1'
  ./${OUT_FILE} '2' 'V' '2'
else
  echo "System V crate synchronization"
  ./${OUT_FILE} '1' 'V' '2' & disown
  ./${OUT_FILE} '2' 'V' '2'
  sleep 5
  echo "POSIX crate synchronization"
  ./${OUT_FILE} '1' 'X' '2' & disown
  ./${OUT_FILE} '2' 'X' '2'
fi
