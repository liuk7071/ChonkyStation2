#include <iostream>

#include "Source/ps2.h"

int main()
{
    Ps2 ps2;
    ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\3stars\\3stars\\3stars.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02a\\demo2a.elf");
    //ps2.ee.pc = ps2.memory.LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_01\\demo1.elf");

    while (1) {
        ps2.ee.Step();
    }
}
