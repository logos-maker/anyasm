# anyasm
Generic assembler for 6502 and custom CPU's. Can even compile straight hexcode text to binary files.
Big endian and little endian support, extra usefull if the processor or data is little endian, so bytes in values is reversed for human reading.
8bit, 16bit, 24bit, 32bit, 48bit, 56bit and 64bit support for hex numbers.
```
00 ; A 8bit number
0000 ; A 16bit numer
000000 ; A 24bit number
00000000; A 32bit number
```
This program can be usefull while developing your own CPU's if instructions with op-code followed be operands. So it's mostly usefull for old styles of 8-bit processors.
Can also be usefull when hacking together binary files for tests, and as a school book example of a assembler.

The assembler and hexcode to binarys is not separated stuff, so you can mix it. Can be usefull if you need to hack together a header for your programs.
So hex numbers can even be given names...
```
#DEFINE border D020
#DEFINE background D021
```
...this makes it possible to make a include file for your custom mnemonics for your CPU or whatever.

It supports include files and decimal number if you put a - or + before the nummer like +10 would generate the same as 0F would.
It can even include binary files, for easy inclusion of data.
```
!mnemonics ; includes the code file mnemonics
!routine.asm ; includes the code file routine.asm
%data.bin ; includes the binary file named data.bin
```
The assembler/program is 500 lines of code, so it was intionally made simple, and was a quick hack to test the concept.
And it could have been simpler if it was not so generic, or a bit more complex to give it more evolved syntax. 
Check the C source code for more hints of what it can do. Obviously it has support for labels in a style like..
```
mylabel:
```
when you later write
```
mylabel
```
...it just inserts the address where mylabel: was crated.

It supports 2 types of style for comments
```
STA 0400 // Letter in the upper left corner of the screen
STA D020 ; border color
```
And you can set the adress origin with
```
$0000 ; Sets the address counter in the compiler.
```
## compilation of the assembler
```
gcc anyasm.c -o anyasm
```
## compilation of code / example program
```
./anyasm -MOS6502 c64_hello_for_anyasm.asm hello.prg
```
## compilation of hexfile to binary example
```
./anyasm -little example.hex example.bin
```
## quick help in commandprompt
```
./anyasm
```
