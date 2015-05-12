//============================================================================
// Name        : virtual_led.cpp
// Author      : dzen Tiur
// Version     :
// Copyright   : BSD license
// Description : Hello World in C++, Ansi-style
//============================================================================


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <cstring>

//
// namespace for model
namespace virtual_led_model {

	//
	//
	enum led_state {
		LS_OFF = 0,
		LS_ON
	};

	//
	//
	enum led_color {
		LC_RED = 1,
		LC_GREEN,
		LC_BLUE,
		LC_MAX_COLOR
	};

	//
	//
	int _led_state_ = LS_OFF;
	//
	//
	int _led_color_ = LC_RED;
	//
	//
	int _blinking_rate_ = 0;

	//
	//
	bool led_is_on() {
		//
		//
		return _led_state_ == LS_ON;
	}

	//
	//
	bool check_blinking_rate_acceptible(int _new_blinking_rate_) {
		//
		//
		return (_new_blinking_rate_ >= 0) && (_new_blinking_rate_ <= 5);
	}

	//
	//
	bool set_blinking_rate(int _new_blinking_rate_) {
		//
		// Check led is turned on
		if(!led_is_on()) {
			//
			// Can't set blinking rate while turned off
			return false;
		}
		//
		// Check blinking rate range
		if(!check_blinking_rate_acceptible(_new_blinking_rate_)) {
			//
			// Can't set blinking rate due to value is not acceptable
			return false;
		}
		//
		//
		_blinking_rate_ = _new_blinking_rate_;
		//
		//
		return true;
	}

	//
	//
	bool get_blinking_rate(int* _result_) {
		//
		if(!_result_) {
			//
			// TODO add error logging here
			return false;
		}
		//
		//
		if(!led_is_on()) {
			//
			//
			return false;
		}
		//
		//
		*_result_ = _blinking_rate_;
		//
		//
		return true;
	}

	//
	//
	bool check_led_color_acceptible(int _new_led_color_) {
		//
		// TODO we need reduce cost of adding new LED colors
		//	so, it must be completely defined on one place
		return (_new_led_color_ >= LC_RED) && (_new_led_color_ < LC_MAX_COLOR);
	}

	//
	//
	bool set_led_color(int _new_led_color_) {
		//
		//
		if(!led_is_on()) {
			//
			//
			return false;
		}
		//
		//
		if(!check_led_color_acceptible(_new_led_color_)) {
			//
			//
			return false;
		}
		//
		//
		_led_color_ = _new_led_color_;
		//
		//
		return true;
	}

	//
	//
	bool get_led_color(int* _result_) {
		//
		if(_result_ == NULL) {
			//
			// TODO we must log internal error here
			return false;
		}
		//
		//
		if(!led_is_on()) {
			//
			//
			return false;
		}
		//
		//
		*_result_ = _led_color_;
		//
		//
		return true;
	}

	//
	//
	bool turn_on() {
		//
		//
		_led_state_ = LS_ON;
		//
		//
		return true;
	}

	//
	//
	bool turn_off() {
		//
		//
		_led_state_ = LS_OFF;
		//
		//
		return true;
	}

	//
	//
	bool get_leg_state(int* _result_) {
		//
		if(_result_ == NULL) {
			//
			//
			return false;
		}
		//
		//
		*_result_ = _led_state_;
		//
		//
		return true;
	}

};

//
// namespace for processing commands from client
namespace commands_processing {

	using namespace std;

	//
	//
	bool parse_command(const string& _command_clause_, string& _command_name_,
			string& _argument_) {
		//
		// Search for argument delimiter
		size_t _first_space_pos_ = _command_clause_.find(" ", 0);
		//
		if(_first_space_pos_ == string::npos ) {
			//
			// if no space in line - treat it as command without arguments
			_command_name_ = _command_clause_;
			//
			//
			return true;
		}
		//
		// Process command as normal command with argument
		_command_name_ = _command_clause_.substr(0, _first_space_pos_);
		//
		_argument_ = _command_clause_.substr(_first_space_pos_+1);
		//
		//
		return true;
	}

	//
	//
	namespace command_handlers {

		//
		//
		void prepare_result_base(bool _boolean_result_, string& _result_base_) {
			//
			if(_boolean_result_) {
				//
				//
				_result_base_.assign("OK");
			}
			else {
				//
				//
				_result_base_.assign("FAILED");
			}
		}

		//
		// return semantic - command accepted and processed
		bool try_process_as_set_led_state(const string& _command_,
				const string& _arg_, string& _result_) {
			//
			if(_command_.compare("set-led-state") != 0) {
				//
				//
				return false;
			}
			//
			//
			if(_arg_.compare("on")==0) {
				//
				//
				prepare_result_base(virtual_led_model::turn_on(), _result_);
			}
			else if(_arg_.compare("off")==0) {
				//
				//
				prepare_result_base(virtual_led_model::turn_off(), _result_);
			}
			else {
				//
				// unknown arguments for command
				prepare_result_base(false, _result_);
			}
			//
			//
			return true;
		}

		//
		//
		bool try_process_as_get_led_state(const string& _command_,
				const string& _arg_, string& _result_) {
			//
			if(_command_.compare("get-led-state") != 0) {
				//
				//
				return false;
			}
			//
			//
			int _led_state_;
			//
			bool _boolean_result_ = virtual_led_model::get_leg_state(&_led_state_);
			//
			prepare_result_base(_boolean_result_, _result_);
			//
			if(!_boolean_result_) {
				//
				//
				return true;
			}
			//
			//
			if(_led_state_ == virtual_led_model::LS_ON) {
				//
				_result_.append(" on");
			}
			else if(_led_state_ == virtual_led_model::LS_OFF) {
				//
				_result_.append(" off");
			}
			else {
				//
				// TODO it is ambiguous answer from virtual_led core
				prepare_result_base(false, _result_);
			}
			//
			//
			return true;
		}

		//
		//
		bool try_process_as_set_led_color(const string& _command_,
			const string& _arg_, string& _result_) {
			//
			if(_command_.compare("set-led-color") != 0) {
				//
				//
				return false;
			}
			//
			// mapping from string to led color constant
			int _new_led_color_ = 0;
			//
			if(_arg_.compare("red") == 0) {
				//
				_new_led_color_ = virtual_led_model::LC_RED;
			}
			else if(_arg_.compare("green") == 0) {
				//
				_new_led_color_ = virtual_led_model::LC_GREEN;
			}
			else if(_arg_.compare("blue") == 0) {
				//
				_new_led_color_ = virtual_led_model::LC_BLUE;
			}
			else {
				//
				// unknown led color requested
				prepare_result_base(false, _result_);
				//
				return true;
			}
			//
			//
			prepare_result_base(virtual_led_model::set_led_color(_new_led_color_),
				_result_);
			//
			//
			return true;
		}

		//
		//
		bool try_process_as_get_led_color(const string& _command_,
			const string& _arg_, string& _result_) {
			//
			if(_command_.compare("get-led-color") != 0) {
				//
				//
				return false;
			}
			//
			// getting led color
			int _led_color_ = 0;
			//
			bool _boolean_result_ = virtual_led_model::get_led_color(&_led_color_);
			//
			prepare_result_base(_boolean_result_, _result_);
			//
			if(!_boolean_result_) {
				//
				//
				return true;
			}
			//
			// mapping from led color constant to string
			if(_led_color_ == virtual_led_model::LC_RED) {
				//
				_result_.append(" red");
			}
			else if(_led_color_ == virtual_led_model::LC_GREEN) {
				//
				_result_.append(" green");
			}
			else if(_led_color_ == virtual_led_model::LC_BLUE) {
				//
				_result_.append(" blue");
			}
			else {
				//
				// unknown led color returned
				prepare_result_base(false, _result_);
			}
			//
			//
			return true;
		}

		//
		//
		bool try_process_as_set_led_rate(const string& _command_,
			const string& _arg_, string& _result_) {
			//
			//
			if(_command_.compare("set-led-rate") != 0) {
				//
				//
				return false;
			}
			//
			// reading led rate from arg
			int _requested_rate_value_ = 0;
			//
			if(sscanf(_arg_.c_str(), "%d", &_requested_rate_value_) != 1) {
				//
				// argument is wrong
				prepare_result_base(false, _result_);
				//
				return true;
			}
			//
			//
			prepare_result_base(
					virtual_led_model::set_blinking_rate(_requested_rate_value_),
					_result_);
			//
			//
			return true;
		}

		//
		//
		bool try_process_as_get_led_rate(const string& _command_,
			const string& _arg_, string& _result_) {
			//
			//
			if(_command_.compare("get-led-rate") != 0) {
				//
				//
				return false;
			}
			//
			// reading led rate from virtual_led core
			int _rate_value_ = 0;
			//
			bool _boolean_result_ =
					virtual_led_model::get_blinking_rate(&_rate_value_);
			//
			prepare_result_base(_boolean_result_, _result_);
			//
			if(!_boolean_result_) {
				//
				//
				return true;
			}
			//
			// converting rate integer value to string
			char _buffer_[10];
			//
			if(sprintf(_buffer_, " %d", _rate_value_) <= 0) {
				//
				// NB fail in sprintf
				prepare_result_base(_boolean_result_, _result_);
				//
				return true;
			}
			//
			//
			_result_.append(_buffer_);
			//
			//
			return true;
		}
	};

	//
	//
	bool process_command(const string& _command_clause_, string& _result_) {
		//
		string _command_, _arguments_;
		//
		//
		if(!parse_command(_command_clause_, _command_, _arguments_)) {
			//
			//
			return false;
		}
		//
		//
		typedef bool (*command_handler) (const string& _command_,
				const string& _arg_, string& _result_);
		//
		command_handler _command_handlers_[6] = {
			command_handlers::try_process_as_set_led_state,
			command_handlers::try_process_as_get_led_state,
			command_handlers::try_process_as_set_led_color,
			command_handlers::try_process_as_get_led_color,
			command_handlers::try_process_as_set_led_rate,
			command_handlers::try_process_as_get_led_rate
		};
		//
		// loop thought all command handlers
		for(int _i_ = 0; _i_< 6; _i_++) {
			//
			if(_command_handlers_[_i_](_command_, _arguments_, _result_)) {
				//
				//
				return true;
			}
		}
		//
		// unknown command
		return false;
	}

	//
	//
	int read_command(const char* _command_buffer_, size_t _command_buffer_size_,
			string& _command_clause_) {
		//
		//
		const char* _command_end_ = (const char*)memchr(_command_buffer_, '\n',
			_command_buffer_size_);
		//
		if(_command_end_ == NULL) {
			//
			// command end character not found
			return 0;
		}
		//
		//
		_command_clause_.assign(_command_buffer_, _command_end_-_command_buffer_);
		//
		//
		return _command_end_-_command_buffer_+1;
	}

	//
	//
	namespace tests {

		//
		//
		bool test_command(const char* _command_clause_,
				const char* _expected_result_) {
			//
			//
			string _command_clause2_(_command_clause_), _result_;
			//
			if(!commands_processing::process_command(_command_clause_, _result_)) {
				//
				//
				cout << "command test failed: " << _command_clause_ << endl;
				cout << "\tcommand returned error" << endl;
				//
				return false;
			}
			//
			//
			if(_result_.compare(_expected_result_) != 0) {
				//
				//
				cout << "command test failed: " << _command_clause_ << endl;
				cout << "\tresult expected: " << _expected_result_ << endl;
				cout << "\tresult received: " << _result_.c_str() << endl;
				//
				return false;
			}
			//
			//
			return true;
		}

		//
		//
		struct test_command_case {
			//
			const char* _command_clause_;
			//
			const char* _expected_result_;
			//
		};

		//
		//
		bool test_commands(test_command_case* _commands_, int _commands_number_) {
			//
			//
			bool _result_ = true;
			//
			for(int _i_ = 0; _i_ < _commands_number_; _i_++) {
				//
				//
				bool _partial_result_ = test_command(_commands_[_i_]._command_clause_,
						_commands_[_i_]._expected_result_);
				//
				_result_ = _result_ && _partial_result_;
			}
			//
			//
			return _result_;
		}

		//
		//
		bool run_complex_test01() {
			//
			//
			test_command_case _commands_[] = {
				{"set-led-state on", "OK"},
				{"set-led-color green", "OK"},
				{"set-led-rate 4", "OK"},
				{"set-led-rate 0", "OK"},
				{"set-led-rate 12", "FAILED"},
				{"set-led-rate -3", "FAILED"},
				{"set-led-color red", "OK"},
				{"get-led-color", "OK red"},
				{"get-led-state", "OK on"},
				{"set-led-color yellow", "FAILED"},
				{"get-led-rate", "OK 0"},
				{"set-led-color blue", "OK"},
				{"set-led-state off", "OK"},
				{"set-led-color red", "FAILED"},
				{"get-led-state", "OK off"},
				{"get-led-rate", "FAILED"},
				{"set-led-rate 3", "FAILED"},
				{"get-led-color", "FAILED"},
			};
			//
			//
			return test_commands(_commands_,
				sizeof(_commands_)/sizeof(_commands_[0]));
		}

	};
}

using namespace std;

#define FUNC_FAILED( message ) cerr << __FUNCTION__ << " " << message << endl;

//
//
namespace system_interaction {

	//
	//
	int _fifo_in_fd_ = 0;
	//
	//
	int _fifo_out_fd_ = 0;

	//
	//
	bool open_fifos() {
		//
		//
		const char _in_fifo_name_[] = "/tmp/virtual_led_control_fifo_in";
		//
		if(mkfifo(_in_fifo_name_, 0666) < 0) {
			//
			// in case when fifo already exists
			if(errno != EEXIST) {
				//
				//
				return false;
			}
		}
		//
		//
		const char _out_fifo_name_[] = "/tmp/virtual_led_control_fifo_out";
		//
		if(mkfifo(_out_fifo_name_, 0666) < 0) {
			//
			// in case when fifo already exists
			if(errno != EEXIST) {
				//
				//
				return false;
			}
		}
		//
		//
		_fifo_in_fd_ = open(_in_fifo_name_, O_RDWR);
		//
		_fifo_out_fd_ = open(_out_fifo_name_, O_RDWR);
		//
		if(_fifo_in_fd_ < 0 || _fifo_out_fd_ < 0) {
			//
			//
			return false;
		}
		//
		//
		return true;
	}

	//
	//
	bool command_processing_loop() {
		//
		//
		const size_t _buffer_size_ = 1024;
		//
		size_t _buffer_content_length_ = 0;
		//
		char _buffer_[_buffer_size_];
		//
		//
		while(true) {
			//
			// reading first portion of data
			size_t _bytes_read_ = read(_fifo_in_fd_, _buffer_+_buffer_content_length_, _buffer_size_-_buffer_content_length_);
			//
			if(_bytes_read_ < 0) {
				//
				//
				FUNC_FAILED("failed on read()");
				//
				return 0;
			}
			//
			if(_bytes_read_ == 0) {
				//
				// connection finished
				_buffer_content_length_ = 0;
				//
				continue;
			}
			//
			// affecting buffer offset due to read data amount
			_buffer_content_length_ += _bytes_read_;
			//
			// try reading command from buffer
			string _command_;
			//
			size_t _command_size_read_ = commands_processing::read_command(_buffer_, _buffer_size_,
					_command_);
			//
			if(_command_size_read_ > 0) {
				//
				// moving buffer content to buffer begin
				memmove(_buffer_, _buffer_+_command_size_read_,
						_buffer_content_length_-_command_size_read_);
				//
				_buffer_content_length_ -= _command_size_read_;
				//
				// command is read
				string _command_result_;
				//
				if(!commands_processing::process_command(_command_,	_command_result_)) {
					//
					//
					_command_result_.assign("FAILED");
				}
				//
				//
				_command_result_.append("\n");
				//
				// Writing output
				if(write(_fifo_out_fd_, _command_result_.c_str(), _command_result_.size()) < 0) {
					//
					//
					FUNC_FAILED("failed on write()");
					//
					return false;
				}
			}
		}
	}
};

//
//
bool fork_as_daemon() {
	//
	int childpid = 0;
	//
	pid_t pid = 0;
	//
	if ((childpid = fork ()) < 0) {
		//
		// check to see if we can get a child
		return false;
	}
	else if (childpid > 0) {
		//
		// if we have a child then parent can exit
		exit(0);
	}
	//
	//Set our sid and continue normal runtime as a forked process
	setsid ();
	//
	// Xxx set the unmask
	umask(0);
	//
	// two ways of retrieving std fdes
	close(fileno(stderr));
	//
	close(fileno(stdout));
	//
	close(STDIN_FILENO);
	//
	//
	return true;
}

//
//
int main() {
	//
	//
	cout << "Virtual LED server started!" << endl; // prints !!!Hello World!!!
	//
	//
	/*if(!fork_as_daemon()) {
		//
		//
		cerr << __FUNCTION__ << " failed on become_daemon()" << endl;
		//
		return 0;
	}*/
	//
	// here we must open named pipe for reading commands
	if(!system_interaction::open_fifos()) {
		//
		//
		FUNC_FAILED("failed on system_interaction::open_fifos()");
		//
		//
		return 0;
	}
	//
	//
	if(!system_interaction::command_processing_loop()) {
		//
		//
		FUNC_FAILED("failed on system_interaction::command_processing_loop()");
		//
		//
		return 0;
	}
	//
	//commands_processing::tests::run_complex_test01();
	//
	//
	return 0;
}
