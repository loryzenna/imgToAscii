# macOS: brew install onnxruntime
# Windows (MinGW): extract onnxruntime-win-x64-*.zip into ./onnxruntime and copy onnxruntime\lib\onnxruntime.dll next to main.exe
ifeq ($(OS),Windows_NT)
ORT = onnxruntime
CC = gcc
# -static: no MinGW DLLs, only onnxruntime.dll is left
STATIC = -static
else
ORT = /opt/homebrew
CC = clang
endif

SRC = main.c load.c subject.c image.c render.c term.c

main: $(SRC) *.h
	$(CC) -O2 -Wall $(SRC) -I$(ORT)/include -I$(ORT)/include/onnxruntime -L$(ORT)/lib -lonnxruntime -lm $(STATIC) -o main
