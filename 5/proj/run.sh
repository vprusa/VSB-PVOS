#!/bin/bash

MAIN_FILE=main.c
OUT_FILE=main

gcc "${MAIN_FILE}" -o "${OUT_FILE}"

#stty -F /dev/tty -icanon

#echo " y jojo nene y3\n" | ./${OUT_FILE}
#echo " y jojo nene y22\n" | ./${OUT_FILE}
#echo " y jojo nene y111\n" | ./${OUT_FILE}
#echo " x jojo nene x\\" | ./${OUT_FILE}
#echo -en " . jojo nene .  \n" | ./${OUT_FILE}
#echo -en " , jojo nene ,  \r" | ./${OUT_FILE}
#echo -en " - jojo nene - " | ./${OUT_FILE}
echo -en " + jojo nene + \n" | ./${OUT_FILE}
echo -en " + jojo nene + " | ./${OUT_FILE}

while true; do echo -en "! jojo nene ?"; sleep 1; done | ./${OUT_FILE}
