
all: virtual-led

virtual-led: virtual-led.cpp
	gcc virtual-led.cpp -g3 -lstdc++ -I/usr/X11R6/include -I/usr/X11R6/include/X11 -L/usr/X11R6/lib -L/usr/X11R6/lib/X11 -lX11 -lpthread -o virtual-led 

clean: 
	rm virtual-led
