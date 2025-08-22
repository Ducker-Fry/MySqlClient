#include<iostream>
#include<input/input_console.h>

int main()
{
    InputData inputData;
    ConsoleInputSource consoleInput;
    inputData = consoleInput.readInput();
    std::cout << inputData.getRawData() << std::endl;
    return 0;
}