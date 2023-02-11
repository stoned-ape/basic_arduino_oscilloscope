#--fqbn=avr-0.0.3:avr:lpleonardo

all: serial.exe upload



upload: arduino\arduino.ino 
	arduino-cli compile -b arduino:avr:leonardo arduino
	arduino-cli upload -b arduino:avr:leonardo -p COM5 -l serial arduino
	touch upload
	sleep 1

serial.exe: serial.c
	cl serial.c

run: serial.exe upload
	serial.exe