# Linux: install onnxruntime
# macOS: brew install onnxruntime
# Windows (MinGW): estrai onnxruntime-win-x64-*.zip in ./onnxruntime e copia onnxruntime\lib\onnxruntime.dll accanto a main.exe
ifeq ($(OS),Windows_NT) # Windows
ORT = onnxruntime
CC = gcc
# -static: niente DLL di MinGW, resta solo onnxruntime.dll
STATIC = -static
else
UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin) # MacOs
ORT = /opt/homebrew
CC = clang
else # Linux
ORT = /usr
CC ?= gcc
endif
endif

main: main.c stb_image.h
	$(CC) -O2 main.c -I$(ORT)/include -I$(ORT)/include/onnxruntime -L$(ORT)/lib -lonnxruntime -lm $(STATIC) -o main
