#!/bin/bash
# 
# checking server is started 
#virtual_led_pid=$(pidof virtual-led)

#if [[ -z $virtual_led_pid ]]; then
#./virtual-led&
#fi

#
# processing arguments
while [[ $# > 0 ]]
do
key="$1"

case $key in
    -c|--color)
    COLOR="$2"
    shift # past argument
    ;;
    -b|--blinkink-freq)
    BLINKING_RATE="$2"
    shift # past argument
    ;;
    -s|--state)
    STATE="$2"
    shift # past argument
    ;;
    *)
          # unknown option
    ;;
esac
shift # past argument or value
done



command_pipe=/tmp/virtual_led_control_fifo_in

#
# processing state
if [[ "$STATE" = "on" ]]; then
    echo "set-led-state on"$'\n' > $command_pipe
else if [[ "$STATE" = "off" ]]; then
    echo "set-led-state off"$'\n' > $command_pipe
else if [[ ! -z "$STATE" ]]; then
    echo "unknown value for --state argument"
fi fi fi

#
# processing color
case $COLOR in 
    R|red|r)
    echo "set-led-color red"$'\n' > $command_pipe
    ;;
    G|green|g)
    echo "set-led-color green"$'\n' > $command_pipe
    ;;
    B|blue|b)
    echo "set-led-color blue"$'\n' > $command_pipe
    ;;
    "")
    ;;
    *)
    echo "unknown color for --color argument"
    ;;
esac

#
# processing blinking frequency
if [[ "$BLINKING_RATE" -ge "0" && "$BLINKING_RATE" -le "5" ]]; then
    echo "set-led-rate $BLINKING_RATE"$'\n' > $command_pipe
else if [[ ! -z $BLINKING_RATE ]]; then
    echo "blinking frequency is out of limits [0;5]"
fi fi

#
# retrieving led info


echo "script done"

