#!/bin/bash

# initialize variables
next_index=1
declare -A pids
declare -A widths
declare -A heights

# function to launch a new cellular automaton process
function launch_cell {
    # get width and height arguments
    width=$1
    height=$2
    
    # launch process with given dimensions and assign index
    ./Version3/cell $WIDTH $HEIGHT | sed -e "s/^/$next_index: /" &
    pids["$next_index"]=$!
    widths["$next_index"]=$width
    heights["$next_index"]=$height
    
    # increment index for next process
    next_index=$((next_index+1))
}

# loop to read and process user commands
while true; do
    read -p "> " cmd

    # extract index and command from input
    index=${cmd%%:*}
    cmd=${cmd#*: }
    
    # check if index is valid
    if [[ -z ${pids[$index]} ]]; then
        echo "Invalid index: $index"
        continue
    fi
    
    # check if process is still running
    if ! kill -0 ${pids[$index]} 2>/dev/null; then
        echo "Process $index has terminated"
        unset pids[$index]
        unset widths[$index]
        unset heights[$index]
        continue
    fi
    
    # process command for selected process
    case "$cmd" in
        "end")
            kill ${pids[$index]}
            ;;
        "faster")
            echo "faster" > /proc/${pids[$index]}/fd/0
            ;;
        "slower")
            echo "slower" > /proc/${pids[$index]}/fd/0
            ;;
        "rule "*)
            rule=${cmd#rule }
            if [[ $rule -ge 0 && $rule -le 255 ]]; then
                echo "rule $rule" > /proc/${pids[$index]}/fd/0
            else
                echo "Invalid rule number: $rule"
            fi
            ;;
        "color on")
            echo "color on" > /proc/${pids[$index]}/fd/0
            ;;
        "color off")
            echo "color off" > /proc/${pids[$index]}/fd/0
            ;;
        "line")
            echo "line" > /proc/${pids[$index]}/fd/0
            ;;
        *)
            echo "Invalid command: $cmd"
            ;;
    esac
done
