NAME := kernel
SRCS := $(shell find . -name "*.c")
export ARCH := x86_64-qemu
export AM_HOME := /root/jyyos/os-workbench/abstract-machine
include $(AM_HOME)/Makefile

debug:
	qemu-system-x86_64 -s -S build/kernel-x86_64-qemu
