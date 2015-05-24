//============================================================================
// Name        : virtual_led.cpp
// Author      : dzen Tiur
// Version     :
// Copyright   : BSD license
// Description : Hello World in C++, Ansi-style
//============================================================================


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <cstring>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <pthread.h>


using namespace std;

#define FUNC_FAILED( message ) cerr << __FUNCTION__ << " " << message << endl;

#define RETURN_FAIL_IF( code, message ) if ( code ) { cerr << __FUNCTION__ << " in file " << __FILE__ << " on line " \
	<< __LINE__ << " " << message << std::endl; return false; }

#define RETURN_FAIL_IF2( code) if ( code ) { return false; }

#define LOG_FAIL_IF( code, message ) if ( code ) { cerr << __FUNCTION__ << " in file " << __FILE__ << " on line " \
	<< __LINE__ << " " << message << std::endl; }


//
// namespace for model
namespace virtual_led_model {

	//
	//
	using namespace std;

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
	led_state _led_state_ = LS_OFF;
	//
	//
	led_color _led_color_ = LC_RED;
	//
	//
	int _blinking_rate_ = 0;

	//
	//
	namespace x11_visualization {

		//
		//
		using namespace std;

		//
		// visualization thread
		pthread_t _visualization_thread_ = 0;
		//
		//
		pthread_t _blinking_thread_ = 0;
		//
		//
		led_color _visualization_color_ = LC_RED;
		//
		// Flag: is LED visualization turned on
		led_state _visualization_state_ = LS_OFF;
		//
		//
		int _visualization_rate_ = 0;
		//
		// Flag: is LED currently lights(1) or faded(0)
		int _blinking_state_ = 1;
		//
		// X11 stuff
		Display* _display_ = NULL;
		//
		Window _window_ = None;
		//
		GC _gc_ = None;
		//
		// visualization x11 colors
		XColor _red_color_ = {0, 64000, 0, 0, DoRed | DoGreen | DoBlue};
		//
		XColor _green_color_ = {0, 0, 64000, 0, DoRed | DoGreen | DoBlue};
		//
		XColor _blue_color_ = {0, 0, 0, 64000, DoRed | DoGreen | DoBlue};
		//
		XColor _gray_color_ = {0, 58000, 58000, 58000, DoRed | DoGreen | DoBlue};
		//
		//
		Atom WM_DELETE_WINDOW = 0;
		//
		//
		pthread_mutex_t _visualization_data_mutex_ = PTHREAD_MUTEX_INITIALIZER;

		//
		// tool function to be called from visualization_thread()
		bool redraw_visualization() {
			//
			// selecting appropriate color
			unsigned long int _selected_color_pixel_id_ = _red_color_.pixel;
			//
			//
			RETURN_FAIL_IF((pthread_mutex_lock(&_visualization_data_mutex_) != 0),
					"failed on pthread_mutex_lock()");
			//
			switch(_visualization_color_) {
				//
				case LC_RED: _selected_color_pixel_id_ = _red_color_.pixel; break;
				//
				case LC_GREEN: _selected_color_pixel_id_ = _green_color_.pixel; break;
				//
				case LC_BLUE: _selected_color_pixel_id_ = _blue_color_.pixel; break;
				//
				default:
					//
					FUNC_FAILED(" failed due to unsupported visualization color");
					//
					//
					return false;
			};
			//
			// If LED is faded while blinking or turned off - we must set gray color
			if(_blinking_state_ == 0 || _visualization_state_ == LS_OFF) {
				//
				_selected_color_pixel_id_ = _gray_color_.pixel;
			}
			//
			RETURN_FAIL_IF(pthread_mutex_unlock(&_visualization_data_mutex_)!=0,
					"failed on pthread_mutex_unlock()");
			//
			// Setting foreground color
			XSetForeground(_display_, _gc_, _selected_color_pixel_id_);
			//
			//
			XFillRectangle(_display_, _window_, _gc_, 5, 5, 25, 25);
			//
			//
			LOG_FAIL_IF(!XFlush(_display_),
					"have fail on XFlush()");
			//
			//
			return true;
		}

		//
		//
		void* visualization_thread(void* _arg_) {
			//
			//
			XEvent e;
			//
			while (1) {
				//
				XNextEvent(_display_, &e);
				//
				if (e.type == Expose) {
					//
					LOG_FAIL_IF(!redraw_visualization(),
							"have fail on redraw_visualization()");
				}
				//
				if ((e.type == ClientMessage) &&
						(static_cast<unsigned int>(e.xclient.data.l[0]) == WM_DELETE_WINDOW)) {
					//
					break;
				}
			}
			//
			//
			return 0;
		}

		//
		//
		bool send_x11_update_event() {
			//
			//
			_XEvent _event_;
			//
			memset(&_event_, 0, sizeof(_event_));
			//
			_event_.type = Expose;
			//
			_event_.xexpose.window = _window_;
			//
			RETURN_FAIL_IF(!XSendEvent(_display_, _window_, False, ExposureMask, &_event_),
					"failed on XSendEvent()");
			//
			RETURN_FAIL_IF(!XFlush(_display_),
					"failed on XFlush()");
			//
			//
			return true;
		}

		//
		// changes blinking state according to rate
		//	generates x11 expose events when blinking state changes
		void* blinking_thread(void*) {
			//
			//
			while(1) {
				//
				// Getting current blinking rate
				assert(!pthread_mutex_lock(&_visualization_data_mutex_));
				//
				int _current_blinking_rate_ = _blinking_rate_;
				//
				assert(!pthread_mutex_unlock(&_visualization_data_mutex_));
				//
				// calculating current sleep period
				if(_current_blinking_rate_ == 0) {
					//
					// Blinking is off
					break;
				}
				//
				int _sleep_period_ = 1000000 / (_current_blinking_rate_*2);
				//
				// cancellation point
				assert(!usleep (_sleep_period_));
				//
				// Now we turn blinking state
				assert(!pthread_mutex_lock(&_visualization_data_mutex_));
				//
				_blinking_state_ = _blinking_state_ ? 0 : 1;
				//
				assert(!pthread_mutex_unlock(&_visualization_data_mutex_));
				//
				// Sending expose event after changing blinking state
				LOG_FAIL_IF(!send_x11_update_event(),
						"have fail on send_x11_update_event()");
			}
			//
			//
			return NULL;
		}

		//
		// Called on each error
		int x11_error_handler(Display* _display_, XErrorEvent* _error_) {
			//
			//
			cerr << __FUNCTION__ << " have catched error from x11: " << endl;
			//
			// Preparing error message
			char _error_mesage_buffer_[10240];
			//
			if(!XGetErrorText(_display_, _error_->error_code, _error_mesage_buffer_, 10240)) {
				//
				FUNC_FAILED("have fail on XGetErrorText(). Error message cannot be obtained.");
			}
			else {
				//
				cerr << _error_mesage_buffer_ << endl;
			}
			//
			// Return value will be ignored - it is x11 design
			return 0;
		}

		//
		// Called only in fatal conditions - such as server connection is lost
		int x11_io_error_handler(Display* _display_) {
			//
			// Return will terminate program
			FUNC_FAILED("reports fatal error: connection to xserver is lost. Program will be terminated.");
			//
			//
			return 0;
		}

		//
		//
		bool init() {
			//
			// Setting Xlib error handler
			RETURN_FAIL_IF(!XSetErrorHandler(x11_error_handler),
					"failed on XSetErrorHandler");
			//
			// Setting Xlib IO error handler
			RETURN_FAIL_IF(!XSetIOErrorHandler(x11_io_error_handler),
					"failed on XSetIOErrorHandler()");
			//
			// Preparing Xlib for multithreading environment
			RETURN_FAIL_IF(!XInitThreads(),
					"failed on XInitThreads()");
			//
			//
			RETURN_FAIL_IF((_display_ = XOpenDisplay(NULL)) == NULL,
					"failed on XOpenDisplay()");
			//
			//
			int s = DefaultScreen(_display_);
			//
			_window_ = XCreateSimpleWindow(_display_, RootWindow(_display_, s), 10, 10, 400, 80, 1,
								   BlackPixel(_display_, s), WhitePixel(_display_, s));
			//
			RETURN_FAIL_IF(_window_ == None,
					"failed on XCreateSimpleWindow()");
			//
			//
			RETURN_FAIL_IF(!XSelectInput(_display_, _window_, ExposureMask),
					"failed on XSelectInput()");
			//
			//
			RETURN_FAIL_IF(!XMapWindow(_display_, _window_),
					"failed on XMapWindow()");
			//
			//
			RETURN_FAIL_IF(!XStoreName(_display_, _window_, "Virtual LED visualization"),
					"failed on XStoreName()");
			//
			// Allocating atom WM_DELETE_WINDOW
			WM_DELETE_WINDOW = XInternAtom(_display_, "WM_DELETE_WINDOW", False);
			//
			RETURN_FAIL_IF(WM_DELETE_WINDOW == None,
					"failed on XInternAtom()");
			//
			//
			RETURN_FAIL_IF(!XSetWMProtocols(_display_, _window_, &WM_DELETE_WINDOW, 1),
					"failed on XSetWMProtocols()");
			//
			// Getting graphic context
			_gc_ = XCreateGC(_display_, _window_, NULL, NULL);
			//
			RETURN_FAIL_IF(_gc_ == None,
					"failed on XCreateGC()");
			//
			// Allocating colors
			if(!XAllocColor(_display_, DefaultColormap(_display_, s), &_red_color_) ||
					!XAllocColor(_display_, DefaultColormap(_display_, s), &_green_color_) ||
					!XAllocColor(_display_, DefaultColormap(_display_, s), &_blue_color_) ||
					!XAllocColor(_display_, DefaultColormap(_display_, s), &_gray_color_)) {
				//
				FUNC_FAILED("failed on XAllocColor()");
				//
				//
				return false;
			}
			//
			// Starting visualization thread
			if(_visualization_thread_ == 0) {
				//
				RETURN_FAIL_IF(0 != pthread_create(&_visualization_thread_, NULL, visualization_thread, NULL),
						"failed on pthread_create()");
			}
			//
			// Starting blinking thread
			if(_blinking_thread_ == 0) {
				//
				RETURN_FAIL_IF(0 != pthread_create(&_blinking_thread_, NULL, blinking_thread, NULL),
					"failed on pthread_create() 2");
			}
			//
			//
			return true;
		}

		//
		//
		bool signal_led_change(led_color _new_led_color_, led_state _new_led_state_, int _new_led_rate_) {
			//
			assert(pthread_mutex_lock(&_visualization_data_mutex_));
			//
			// Preserving old values
			led_color _old_color_ = _visualization_color_;
			//
			led_state _old_state_ = _visualization_state_;
			//
			int _old_rate_ = _visualization_rate_;
			//
			// Restarting blinking thread
			if(_new_led_rate_ != _old_rate_) {
				//
				// 	We finish old blinking thread only if it actually was set
				if(_old_rate_ != 0) {
					//
					if(pthread_cancel(_blinking_thread_)) {
						//
						FUNC_FAILED(" failed on pthread_cancel()");
						//
						goto FAIL_END;
					}
					//
					// Releasing mutex to let blinking_thread() finish
					assert(!pthread_mutex_unlock(&_visualization_data_mutex_));
					//
					RETURN_FAIL_IF(pthread_join(_blinking_thread_, NULL),
							"failed on pthread_join()");
					//
					assert(!pthread_mutex_lock(&_visualization_data_mutex_));
				}
				//
				// We start blinking thread only if blinking actually set
				if(_new_led_rate_ != 0) {
					//
					if(pthread_create(&_blinking_thread_, NULL, blinking_thread, NULL)) {
						//
						FUNC_FAILED(" failed on pthread_create()");
						//
						goto FAIL_END;
					}
				}
			}
			//
			// Setting internal variables
			_visualization_color_ = _new_led_color_;
			//
			_visualization_state_ = _new_led_state_;
			//
			_visualization_rate_ = _new_led_rate_;
			//
			// update visualization
			LOG_FAIL_IF(!send_x11_update_event(),
					" have fail on send_x11_update_event()");
			//
			assert(!pthread_mutex_unlock(&_visualization_data_mutex_));
			//
			//
			return true;

		FAIL_END:
			//
			assert(!pthread_mutex_unlock(&_visualization_data_mutex_));
			//
			//
			return false;
		}

		//
		// visualization isn't restartable
		bool deinit() {
			//
			bool _result_ = true;
			//
			assert(!pthread_mutex_lock(&_visualization_data_mutex_));
			//
			// Stopping threads
			if(pthread_cancel(_visualization_thread_) == 0) {
				//
				//
				if(pthread_join(_visualization_thread_, NULL)) {
					//
					FUNC_FAILED("have fail on pthread_join()");
					//
					_result_ = false;
				}
			}
			else {
				//
				FUNC_FAILED("have fail on pthread_cancel()");
				//
				_result_ = false;
			}
			//
			//
			if(pthread_cancel(_blinking_thread_) == 0) {
				//
				//
				if(pthread_join(_blinking_thread_, NULL)) {
					//
					FUNC_FAILED("have fail on pthread_join() 2");
					//
					_result_ = false;
				}
			}
			else {
				//
				FUNC_FAILED("failed on pthread_cancel() 2");
				//
				_result_ = false;
			}
			//
			//
			if(pthread_mutex_destroy(&_visualization_data_mutex_)) {
				//
				FUNC_FAILED("have fail on pthread_mutex_destroy()");
				//
				_result_ = false;
			}
			//
			// Releasing x11 resources
			if(!XFreeGC(_display_, _gc_)) {
				//
				FUNC_FAILED("have fail on XFreeGC()");
				//
				_result_ = false;
			}
			//
			if(!XDestroyWindow(_display_, _window_)) {
				//
				FUNC_FAILED("have fail on XDestroyWindow()");
				//
				_result_ = false;
			}
			//
			if(!XCloseDisplay(_display_)) {
				//
				FUNC_FAILED("have fail on XCloseDisplay()");
				//
				_result_ = false;
			}
			//
			//
			return _result_;
		}
	};

	namespace x11_visualization_one_thread_approach {

		//
		//
		using namespace std;

		//
		// visualization thread
		pthread_t _visualizaton_thread_ = 0;
		//
		//
		led_color _visualization_color_ = LC_RED;
		//
		//
		int _blink_rate_ = 0;
		//
		// is LED visualization turned on
		led_state _state_ = LS_OFF;
		//
		// is LED currently lights(1) or faded(0)
		int _blinking_state_ = 1;
		//
		//
		Display* _display_ = NULL;
		//
		// visualization x11 colors
		XColor _red_color_ = {0, 64000, 0, 0, DoRed | DoGreen | DoBlue};
		//
		XColor _green_color_ = {0, 0, 64000, 0, DoRed | DoGreen | DoBlue};
		//
		XColor _blue_color_ = {0, 0, 0, 64000, DoRed | DoGreen | DoBlue};
		//
		XColor _blinking_fade_color_ = {0, 48000, 48000, 48000, DoRed | DoGreen | DoBlue};

		//
		//
		bool my_XNextEventTimed(Display* dsp, XEvent* event_return, struct timeval*	tv) {
			//
			// Check for timeout is set
			if (tv == NULL) {
				//
				//
				XNextEvent(dsp, event_return);
				//
				//
				return true;
			}
			//
			// Check for any events already available
			if (XPending(dsp) != 0) {
				//
				//
				XNextEvent(dsp, event_return);
				//
				//
				return true;
			}
			//
			// Waiting for connection to x server
			int fd = ConnectionNumber(dsp);
			//
			fd_set readset;
			//
			FD_ZERO(&readset);
			//
			FD_SET(fd, &readset);
			//
			if (select(fd+1, &readset, NULL, NULL, tv) == 0) {
				//
				// Waiting is timed out
				return false;
			}
			//
			//
			XNextEvent(dsp, event_return);
			//
			//
			return true;
		}

		//
		//
		bool redraw_visualization(Display* dpy, Window win, GC _gc_) {
			//
			// selecting appropriate color
			unsigned long int _selected_color_pixel_id_ = _red_color_.pixel;
			//
			// NB need synchronization here
			switch(_visualization_color_) {
				//
				case LC_RED: _selected_color_pixel_id_ = _red_color_.pixel; break;
				//
				case LC_GREEN: _selected_color_pixel_id_ = _green_color_.pixel; break;
				//
				case LC_BLUE: _selected_color_pixel_id_ = _blue_color_.pixel; break;
				//
				default:
					//
					//
					cerr << __FUNCTION__ << "unsupported visualization color. " << endl;
					//
					exit(0);
			};
			//
			// Setting foreground color
			if(XSetForeground(dpy, _gc_, _selected_color_pixel_id_) == 0) {
				//
				//
				cerr << __FUNCTION__ << "failed on setting foreground color. " << endl;
				//
				exit(0);
			}
			//
			//
			XFillRectangle(dpy, win, _gc_, 5, 5, 25, 25);
			//
			//
			return true;
		}

		//
		//
		bool wait_and_process_event() {
			//
			// select type of event to wait for:
			//
			// Check for any x11 events already available
			if (XPending(_display_) != 0) {
				//
				//goto PROCESS_X11_EVENT;
			}
			//
			// Preparing common set of fd's to wait for: x11 connection fd, timer fd
			fd_set _fds_set_;
			//
			FD_ZERO(&_fds_set_);
			//
			//
			int _x11_connection_fd_ = ConnectionNumber(_display_);
			//
			FD_SET(_x11_connection_fd_, &_fds_set_);
			//
			//
			//timeval _timeout_value_ = { 0, 100000 };
			//
			//FD_SET(_blinking_timer_fd_, &_fds_set_);
			//
			// Waiting for connection to x server or timer event
			//timeval _timeout_value_ = { 0, 10000 };
			//
			/*if (select(fd+1, &_fds_set_, NULL, NULL, &_timeout_value_) == 0) {
				//
				// Waiting is timed out
				return false;
			}*/
			//
			//
			//
			//
			//PROCESS_X11_EVENT:
				//
				//
				//return process_x11_event();
		}

		//
		//
		void* visualization_thread(void* _arg_) {
			//
			//
			cout << __FUNCTION__ << " executed" << endl;
			//
			//
			/*dpy = XOpenDisplay(NULL);
			//
			if (dpy == NULL) {
				//
				cerr << "Cannot open display\n";
				//
				//
				exit(1);
			}
			//
			int s = DefaultScreen(dpy);
			//
			Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, s), 10, 10, 50, 50, 1,
								   BlackPixel(dpy, s), WhitePixel(dpy, s));
			//
			XSelectInput(dpy, win, ExposureMask | KeyPressMask);
			//
			XMapWindow(dpy, win);
			//
			XStoreName(dpy, win, "Virtual LED visualization");
			//
			Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
			//
			//
			XSetWMProtocols(dpy, win, &WM_DELETE_WINDOW, 1);
			//
			// Getting graphic context
			GC _gc_ = XCreateGC(dpy, win, NULL, NULL);*/
			//
			// TODO we must check GC creation result
			/*if(_gc_) {
				//
				//
			}*/
			//
			// Allocating colors
			/*if(XAllocColor(dpy, DefaultColormap(dpy, s), &_red_color_) == 0 ||
					XAllocColor(dpy, DefaultColormap(dpy, s), &_green_color_) == 0 ||
					XAllocColor(dpy, DefaultColormap(dpy, s), &_blue_color_) == 0 ||
					XAllocColor(dpy, DefaultColormap(dpy, s), &_blinking_fade_color_) == 0) {
				//
				//
				cerr << "Can't obtain color." << endl;
				//
				exit(0);
			}*/
			//
			//
			bool uname_ok = false;
			//
			struct utsname sname;
			//
			int ret = uname(&sname);
			//
			if (ret != -1) {
				//
				uname_ok = true;
			}
			//
			XEvent e;
			//
			while (1) {
				//
				wait_and_process_event();
				//my_XNextEventTimed(dpy, &e, &_timeout_value_);
				//
				if (e.type == Expose) {
					//
					//redraw_visualization(dpy, win, _gc_);
				}
				//
				if (e.type == KeyPress) {
					//
					char buf[128] = {0};
					//
					KeySym keysym;
					//
					int len = XLookupString(&e.xkey, buf, sizeof buf, &keysym, NULL);
					//
					if (keysym == XK_Escape) {
						break;
					}
				}
				//
				/*if ((e.type == ClientMessage) &&
						(static_cast<unsigned int>(e.xclient.data.l[0]) == WM_DELETE_WINDOW)) {
					//
					break;
				}*/
			}
			//
			//
			//XFreeGC(dpy, _gc_);
			//
			//XDestroyWindow(dpy, win);
			//
			//XCloseDisplay(dpy);
			//
			//
			return 0;
		}

		//
		//
		bool start(led_color _new_color_, int _new_rate_) {
			//
			// If visualization thread exists -
			if(0 != pthread_create(&_visualizaton_thread_, NULL, visualization_thread, NULL)) {
				//
				//
				return false;
			}

		}

		//
		//
		bool stop() {
			//;
		}
	};


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
		//	Can't set blinking rate while turned off
		RETURN_FAIL_IF2(!led_is_on());
		//
		// Check blinking rate range
		//	Can't set blinking rate due to value is not acceptable
		RETURN_FAIL_IF2(!check_blinking_rate_acceptible(_new_blinking_rate_));
		//
		//
		_blinking_rate_ = _new_blinking_rate_;
		//
		//
		LOG_FAIL_IF(!x11_visualization::signal_led_change(_led_color_, _led_state_, _blinking_rate_),
				"have fail on x11_visualization::signal_led_change()");
		//
		//
		return true;
	}

	//
	//
	bool get_blinking_rate(int* _result_) {
		//
		RETURN_FAIL_IF(!_result_,
				"failed due to wrong arguments");
		//
		//
		RETURN_FAIL_IF2(!led_is_on());
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
		RETURN_FAIL_IF2(!led_is_on());
		//
		//
		RETURN_FAIL_IF2(!check_led_color_acceptible(_new_led_color_));
		//
		//
		_led_color_ = (led_color)_new_led_color_;
		//
		//
		LOG_FAIL_IF(!x11_visualization::signal_led_change(_led_color_, _led_state_, _blinking_rate_),
				"have fail on x11_visualization::signal_led_change()");
		//
		//
		return true;
	}

	//
	//
	bool get_led_color(int* _result_) {
		//
		RETURN_FAIL_IF(_result_ == NULL,
				"failed due to wrong arguments");
		//
		//
		RETURN_FAIL_IF2(!led_is_on());
		//
		//
		*_result_ = _led_color_;
		//
		//
		return true;
	}

	//
	//
	bool init() {
		//
		RETURN_FAIL_IF(!x11_visualization::init(),
				"failed on x11_visualization::init()");
		//
		//
		return true;
	}

	//
	//
	bool deinit() {
		//
		RETURN_FAIL_IF(!x11_visualization::deinit(),
				"failed on x11_visualization::deinit()");
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
		LOG_FAIL_IF(!x11_visualization::signal_led_change(_led_color_, _led_state_, _blinking_rate_),
				"have fail on x11_visualization::signal_led_change()");
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
		LOG_FAIL_IF(!x11_visualization::signal_led_change(_led_color_, _led_state_, _blinking_rate_),
				"have fail on x11_visualization::signal_led_change()");
		//
		//
		return true;
	}

	//
	//
	bool get_led_state(int* _result_) {
		//
		RETURN_FAIL_IF(_result_ == NULL,
				"failed due to wrong arguments");
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
			bool _boolean_result_ = virtual_led_model::get_led_state(&_led_state_);
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
		RETURN_FAIL_IF(!parse_command(_command_clause_, _command_, _arguments_),
				"failed on parse_command()");
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
	// return size of accepted part of buffer
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
			RETURN_FAIL_IF(errno != EEXIST,
					"can't create input fifo");
		}
		//
		//
		const char _out_fifo_name_[] = "/tmp/virtual_led_control_fifo_out";
		//
		if(mkfifo(_out_fifo_name_, 0666) < 0) {
			//
			// in case when fifo already exists
			RETURN_FAIL_IF(errno != EEXIST,
					"can't create output fifo");
		}
		//
		//
		_fifo_in_fd_ = open(_in_fifo_name_, O_RDWR);
		//
		_fifo_out_fd_ = open(_out_fifo_name_, O_RDWR);
		//
		RETURN_FAIL_IF(_fifo_in_fd_ < 0 || _fifo_out_fd_ < 0,
				"can't open one or both required fifo's");
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
			RETURN_FAIL_IF(_bytes_read_ < 0,
					"failed on read()");
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
				RETURN_FAIL_IF(write(_fifo_out_fd_, _command_result_.c_str(), _command_result_.size()) < 0,
						"failed on write()");
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
	cout << "Virtual LED server started!" << endl;
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
	// initializing virtual led model
	if(!virtual_led_model::init()) {
		//
		FUNC_FAILED("failed on virtual_led_model::init()");
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
