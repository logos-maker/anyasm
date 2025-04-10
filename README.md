# anyasm
Generic assembler for 6502 and custom CPU's. Can even compile straight hexcode text to binary files.
Big endian and little endian support, extra usefull if the processor or data is little endian, so bytes in values is reversed for human reading.
8bit, 16bit, 24bit, 32bit, 48bit, 56bit and 64bit support for hex numbers.

Can be usefull while developing your own CPU's if instructions with op-code followed be operands. So it's mostly usefull for old styles of 8-bit processors.
So

The assembler/program is 500 lines of code, so it was intionally made simple.

## compilation of the assembler
gcc anyasm.c -o anyasm

## compilation of code / example program
./anyasm -MOS6502 c64_hello_for_anyasm.asm hello.prg

## quick help in commandprompt
./anyasm

