// A simple hello world program for C64

// A autostart header
801	// BASIC start address / .PRG header, address to load program/data to
80B	// Pointer to next BASIC line
000A	// Line number 10
9E	// 'SYS' token
20	// Space
"2064"	// Hex 810 as text in decimal notation
00	// End of line
0000	// End of BASIC program

0810
$0810		// .ORG for the program code
  CLI
  JSR E544     // Call the Function that clears the screen
  LDX# 00      // Put 0 in Register X (Index Register)
 
write:
  LDA[X] hello  // Read next character from Address at Label .hello + Offeset X
  JSR FFD2      // CHROUT Subroutine, prints the Character loaded into Register A
  INX           // Increments Register X by 1
  CPX# +12      // Compare if Value in Register X equals to 11
  BNE .write    // If Value in Register X is not 11, go back to $033E
  SEI
  RTS           // Return to Basic  
 
hello: "HELLO WORLD!" //48 45 4C 4C 4F 20 57 4F 52 4C 44 21 //"HELLO WORLD!"// "HELLO WORLD"
