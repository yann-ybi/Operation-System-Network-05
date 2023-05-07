#!/bin/bash

# parse arguments
WIDTH=$1
HEIGHT=$2
THREADS=$3

# lauch of a cellular automaton process
./Version3/cell $WIDTH $HEIGHT $THREADS &

# pip setup
PIPE=/tmp/cellpipe
rm -f $PIPE
mkfifo $PIPE

send_cmd() {
    echo "$1" > /proc/$!/fd/0
    read -t 1 RESPONSE < /proc/$!/fd/1
    echo "$RESPONSE"
}


# communicate with the process
while true; do
  read -r CMD

  case "$CMD" in 
    rule*)
      RULE=$(echo "$CMD" | cut -d " " -f 2)
      if [[ $RULE =~ ^[0-9]+$ ]]; then
        send_cmd "$CMD"
      else
        echo "Invalid rule number: $RULE"
      fi
      ;;
    color\ on)
      send_cmd "$CMD"
      ;;
    color\ on)
    send_cmd "$CMD"
      ;;
    color\ off)
    send_cmd "$CMD"
      ;;
    line)
    send_cmd "$CMD"
      ;;
    faster)
    send_cmd "$CMD"
      ;;
    slower)
    send_cmd "$CMD"
      ;;
    end)
    send_cmd "$CMD"
    break
      ;;
    *)
    echo "Invalid command: $CMD"
    ;;
  esac
done

# Clean up named pipe
rm -f $PIPE
