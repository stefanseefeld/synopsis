# Makefile for Perceps demo
TARGET := Manual/index.html
TARGET_D := ManualD/index.html
SOURCE := display.h

all:	$(TARGET)

doxygen: $(TARGET_D)

$(TARGET): $(SOURCE) config.py
	../../synopsis -c config.py -Wc,parser=C++,linker=Linker,formatter=HTML -o Manual display.h

$(TARGET_D): $(SOURCE) config.py
	../../synopsis -c config.py -Wc,parser=C++,linker=Linker,formatter=HTML_Doxygen -o ManualD display.h

clean:
	rm -rf core *~ Manual ManualD
