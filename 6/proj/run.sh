#!/bin/bash

MAIN_FILE=main.c
OUT_FILE=main

gcc "${MAIN_FILE}" -o "${OUT_FILE}"

stty -F /dev/tty -icanon

./${OUT_FILE}

