#!/bin/bash

# Upravte soketový server a klienta, aby bylo možno navazovat spojení na IP4/IP6 a UNIX socketu.
# Příklad github.
# Volba u klienta např. -4, -6 a -u.

# TODO ...
echo "build"
MAIN_CL_FILE=socket_cl.cpp
OUT_CL_FILE=socket_cl

MAIN_SRV_FILE=socket_srv.cpp
OUT_SRV_FILE=socket_srv

gcc "${MAIN_CL_FILE}" -o "${OUT_CL_FILE}"
gcc "${MAIN_SRV_FILE}" -o "${OUT_SRV_FILE}"

echo "build done"

if [[ -n "${1}" && "${1}" = *"test-o"* ]]; then
  echo test
elif [[ -n "${1}" && "${1}" = *"test-u"* ]]; then
echo "start test -u"

MAIN_CL_FILE=socket_cl.cpp
OUT_CL_FILE=socket_cl

MAIN_SRV_FILE=socket_srv.cpp
OUT_SRV_FILE=socket_srv

gcc "${MAIN_CL_FILE}" -o "${OUT_CL_FILE}"
gcc "${MAIN_SRV_FILE}" -o "${OUT_SRV_FILE}"

SOCKET_FILE="/tmp/socket_file"
#  OUT_SRV_FILE=./tmp.srv.log
#  OUT_CL_FILE=./tmp.cl.log
SCREEN_SRV="pvos_9_srv"
SCREEN_CL="pvos_9_cl"

screen -dmS ${SCREEN_SRV} ./${OUT_SRV_FILE} -u "${SOCKET_FILE}"
sleep 1
screen -dmS ${SCREEN_CL} ./${OUT_CL_FILE} -u "${SOCKET_FILE}"
sleep 1

SCREEN_SRV_OUT="pvos_9_srv.out"
SCREEN_CL_OUT="pvos_9_cl.out"
screen -S ${SCREEN_SRV} -X hardcopy ${SCREEN_SRV_OUT}
echo "Server out:"
cat ${SCREEN_SRV_OUT}
screen -S ${SCREEN_CL} -X hardcopy ${SCREEN_CL_OUT}
echo "Client out:"
cat ${SCREEN_CL_OUT}
echo "Sending 'hello' from client"
screen -S ${SCREEN_CL} -p 0 -X stuff "hello^M"
sleep 1
echo "Sending 'hello' from client"
screen -S ${SCREEN_SRV} -X hardcopy ${SCREEN_SRV_OUT}
echo "Server out:"
cat ${SCREEN_SRV_OUT}
echo "Client out:"
cat ${SCREEN_CL_OUT}

RES=`cat ${SCREEN_SRV_OUT}`

if [[ "${REST}" = *"hello"* ]] ; then
  # Echo "OK" in green
  echo -e "\e[32mOK\e[0m"
else
  # Echo "ERR" in red
  echo -e "\e[31mERR\e[0m"
fi

screen -S ${SCREEN_SRV} -X kill
screen -S ${SCREEN_CL} -X kill

#  ${OUT_CL_FILE} -u "${SOCKET_FILE}"
elif [[ -n "${1}" && "${1}" = *"-4"* ]]; then
  ${OUT_SRV_FILE} -6 10090
  ${OUT_CL_FILE} -6 10090
elif [[ -n "${1}" && "${1}" = *"-6"* ]]; then
  ${OUT_SRV_FILE} -4 10090
  ${OUT_CL_FILE} -4 10090
fi

#stty -F /dev/tty -icanon

#./${OUT_FILE} '1' & disown
##./${OUT_FILE} '1'
#./${OUT_FILE} '2'

#if [[ -n "${1}" && "${1}" = *"X"* ]]; then
#  ./${OUT_FILE} '1' 'X' '2' & disown
##  ./${OUT_FILE} '1' 'X' '2'
#  ./${OUT_FILE} '2' 'X' '2'
#elif [[ -n "${1}" && "${1}" = *"V"* ]]; then
#  ./${OUT_FILE} '1' 'V' '2' & disown
#  ./${OUT_FILE} '2' 'V' '2'
#else
#  echo "System V queue"
#  ./${OUT_FILE} '1' 'V' '2' & disown
#  ./${OUT_FILE} '2' 'V' '2'
#  sleep 5
#  echo "POSIX queue"
#  ./${OUT_FILE} '1' 'X' '2' & disown
#  ./${OUT_FILE} '2' 'X' '2'
#fi
