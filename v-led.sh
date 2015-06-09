#!/bin/bash
# 
# checking server is started 
function check_and_make_server_started {
	#
	virtual_led_pid=$(pidof virtual-led)
	#
	if [[ -z $virtual_led_pid ]]; then
	./virtual-led&
	fi
}
#
check_and_make_server_started

command_pipe=/tmp/virtual_led_control_fifo_in
answers_pipe=/tmp/virtual_led_control_fifo_out

#
#
function print_usage {
	#
	echo "usage: v-led.sh [{-s|--state} {on|off}] [{-c|--color} {red|green|blue}] [{-b|--blinking-rate} {0|1|2|3|4|5}]"
	echo ""
}

#
# processing state
function process_state {
	#
	if [[ "$STATE" = "on" ]]; then
		echo "set-led-state on"$'\n' > $command_pipe
	else if [[ "$STATE" = "off" ]]; then
		echo "set-led-state off"$'\n' > $command_pipe
	else if [[ ! -z "$STATE" ]]; then
		echo "ERROR: unknown value for --state argument"
		echo ""
		print_usage
	fi fi fi
}

#
# processing color
function process_color {
	#
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
		echo "ERROR: unknown color for --color argument"
		echo ""
		print_usage
		;;
	esac
}

#
# processing blinking frequency
function process_blinking_frequency {
	#
	if [[ "$BLINKING_RATE" -ge "0" && "$BLINKING_RATE" -le "5" ]]; then
		echo "set-led-rate $BLINKING_RATE"$'\n' > $command_pipe
	else if [[ ! -z $BLINKING_RATE ]]; then
		echo "ERROR: blinking frequency is out of limits [0;5]"
		echo ""
		print_usage
	fi fi
}

#
# processing arguments
while [[ $# > 0 ]]
do
	key="$1"
	#
	case $key in
		-c|--color)
		COLOR="$2"
		process_color
		shift # past argument
		;;
		-b|--blinking-freq)
		BLINKING_RATE="$2"
		process_blinking_frequency
		shift # past argument
		;;
		-s|--state)
		STATE="$2"
		process_state
		shift # past argument
		;;
		*)
		echo "ERROR: unknown option"		
		echo ""
		print_usage
		;;
	esac
	shift # past argument or value
done


#
# dropping all remaining messages from result pipeline
( dd if=$answers_pipe iflag=nonblock of=/dev/null ) > /dev/null 2>&1

#
#
function read_answer {
	#
	local line
	#
	read line <$answers_pipe
	#
	IFS=' ' read -ra ANSWER_TOKENS_ARRAY <<< "$line"
	#
	# checking result 
	if [[ "${ANSWER_TOKENS_ARRAY[0]}" != "OK" ]]; then
		#
		echo "command failed: ${ANSWER_TOKENS_ARRAY[0]}"
		#
		# TODO we must do more elegant error handling here
		exit
	fi
	# 
	if [[ "${#ANSWER_TOKENS_ARRAY[@]}" -ge 2 ]]; then
		#
		eval $1=\"\$\{ANSWER_TOKENS_ARRAY\[1\]\}\"
	else
		#
		unset -v $1
	fi
}

#
# retrieving led info
function retrieve_v_led_info {
	#
	local answer_result
	#
	# checking state is on
	echo "get-led-state" > $command_pipe
	#
	read_answer answer_result
	#
	eval $1=\"\$\{answer_result\}\"
	#
	if [[ "$answer_result" != "on" ]]; then
		#
		unset -v $2
		unset -v $3
		#
		#
		return 
	fi
	#
	# retreiving led color
	echo "get-led-color" > $command_pipe
	#
	read_answer answer_result
	#
	eval $2=\"\$\{\answer_result}\"
	#
	# retreiving led blinking rate
	echo "get-led-rate" > $command_pipe
	#
	read_answer answer_result
	#
	eval $3=\"\$\{\answer_result}\"
}


#
# printing complete led state info
retrieve_v_led_info led_state_info led_color_info led_rate_info
echo "V-LED: state: $led_state_info; color: $led_color_info; blinking freq: $led_rate_info"
echo ""

