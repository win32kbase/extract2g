CC= gcc
OPTS= -Wall -g

ifeq ($(shell uname),WindowsNT)
CCACHE :=
else
CCACHE := $(shell which ccache)
endif

all: extract2g

extract2g: extract2g.c extract2g.h
	$(CCACHE) $(CC) $(OPTS) $< -o $@

.PHONY: clean
clean:
	rm -f extract2g
