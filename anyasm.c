/* 
 anyasm:
 -------
 anyasm is a command line tool for convert/compile hex to binary, and can be used as primitive assembler with funky syntax.
 This peculiar assembly syntax makes the code for this program easy to understand and modify.
 It's not made for any specific processor, and new mnemonics for op-codes can be loaded from include files or staight in your code,
 so in some cases you don't even have to change anything in this program to give support for some processors.
 It has some support for writing 6502 code, and some demos in code on how to give it support for Z80 and 6800.
 But it's probably most suited for 6502 and 8080 to have some similarity close to the processors original syntax.
 It can also act as a bin to hex converter with little and big endian support. <- does not know any other program that compiles hex data to bin data with label support.
 The labels for automatic addresses in steps of bytes, E.g 16bit 24bit 32bit 48bit 56bit or 64bit,
 it's set by the nummber of bytes used in the set origin statement E.g $C000 gives 16bit but $00C000 gives 24bit.

 Why do you want to use this?
 If you are hacking file formats, binary files or need a assembler for your DIY/homebrew/own custom CPU or MCU.
 If you want to make your own assembler and want some code to be inspired of or as a start for your own.
 You can even use it as it is for your custom CPU, just begin your assemblyfile with declaring mnemonics.
 At it's current state it's more suited for old 8-bit CISC processors, as statements set bytes and not bits.

 Supports:
 * Include file support for other binary files with %binfilename or include code in text with !txtfilename
 * Headecimal numbers. 
 * Decimal numbers must have a + or a - infront of it. E.g +1 or -1
 * Comments in code can be done with C/C++ style comments. E.g. text after // on a row is treated as a comment.
 * Text in ASCII inside " " is converted to binary. Even works for some parts of PETSCII
 * Label creation with labels referencing backwards and forward. E.g. labelname: <-creates  labelname <- outputs the couter value where the label was created.
 * Setting of origin (org) for byte counting E.g.  $C000 This sets the global_counter to that value. 
 * Filling/pading output file with zeros. E.g. C010$ will create zeros untill that address/global counter is reached, leaving address C010 hex unfilled. 
 * Big endian and little endian support. Big endian (with a "little end"), Little endian (with a "big end").
 * 8bit, 16bit, 24bit, 32bit, 48bit, 56bit and 64bit support for hex numbers , extra usefull if the processor or data is little endian, so bytes in values is reversed for human reading.
 * 8bit relative labels...    .label gives a relative 8 bit value. And the value is only right if a one byte instruction was before it. Better than nothing. 

 Comments on usage:
 * Data size is set by the number of chars with hex numbers (pad with zeros in begining of number if needed) and is set to a default number of bytes for decimal numbers.
 * The default size for decimal data can be set with 8bit 16bit 24bit 32bit 48bit 56bit 64bit and so on inside the code.
 
 Todo:
 * Need a program to make elf headers for machine code without it, to make tests and make support för a RISC processor like ARM och RISC-V.
   ...ask chat GPT for tips on this... Ask: Is there a command to make ELF headers for binary/machine code without it?
 * Need to select a RISC processor to make code for. Probably Risc-V as it's simple, and give it a go.
 * This was just a quick hack, so there is probably a lot to improve, but the basic functionallity works.
	e.g. maybe it could be good to be able to set the processor type in the source you are assembling.

 Changes:
 v5 changed buffer[8] to a uint64_t and a lot lot more.
 v6 Bugs removed. So we can use the right syntax again.
 v7 Fixed that it prints an error if it don't find any match on input token.

 How to comple it:
  gcc anyasm.c -o anyasm

 Example of command to use is: ...to compile the C64 example program. 
  ./anyasm -MOS6502 c64_hello_for_anyasm.asm hello64.prg


*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define MAX_LABELS 1000 // Max number of labels that can be stored

// Structure to hold a label and its offset
typedef struct {
    char name[50];
    char bytes;
    uint64_t offset;
} Label;

Label labels[MAX_LABELS];
size_t label_count = 0;
uint64_t global_offset = 0; // Global offset initialized to 0
int mnemonic_set = 0; // What mnemonics to use
int pars_again = 0; // To resolve labels pointing forward in code after automaticly been set to one if possible unresolved labels are found.
int pars_round = 0;
int label_bytes = 2; // How many bytes labels should generate. 16bit address bus gives the number 2. $0C000 would change it to 3, $C0 to 2bytes.
int little_endian = 0;
int last_in_bebug = 0; // Give all if/else cases a no. Then you can know where you where last

void add_label(const char *name, uint64_t offset, char bytes) {
    if (label_count >= MAX_LABELS) {
        fprintf(stderr, "Error: Exceeded maximum label count.\n");
        exit(1);
    }
    strncpy(labels[label_count].name, name, sizeof(labels[label_count].name) - 1);
    labels[label_count].offset = offset;
    labels[label_count].bytes  = bytes;
    label_count++;
}

uint64_t find_label(const char *name) {  // Varför kan denna aldrig leverera -1
    for (size_t i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) {
            return labels[i].offset;
        }
    }
    return (uint64_t)-1; // Not found
}

int bytes_label(const char *name) {
    for (size_t i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) {
            return labels[i].bytes;
        }
    }
    return -1; // Not found
}

uint64_t write_bytes(FILE *output, uint64_t *buffer, size_t size, int little_endian) {
    size_t bytes_written = 0;
    if(size > 8){ 
	printf("In write_bytes() the program tries to write %zu bytes to the buffer\n", size);
	for (int i = 0; i < size; i++) {
	    printf("buffer[%d] = %02lX\n", i, buffer[i]);
	}
	exit(1);
    }
    if (little_endian) { // CPU eats from the little end of the egg. Check the first part of the 1726 novel Gulliver's Travels by Jonathan Swift for reference.
        for (size_t i = 0; i < size; i++) { // LSB's (most significant bits) are output first.
            fputc(buffer[size - 1 - i], output);
            bytes_written++; // nr of bytes in number, don't need to be output as a uint64_t but a size_t
        }
    } else { // CPU eats from the big end of the egg
        for (size_t i = 0; i < size; i++) { // MSB's (least significant bits) are output first.
            fputc(buffer[i], output);
            bytes_written++;
        }
    }
    return bytes_written;
}

void skip_whitespace_and_comments(FILE *input,FILE *output) {
    int ch;
    while ((ch = fgetc(input)) != EOF) {
        if (isspace(ch)) {
            continue;
        }
	if (ch == ';') while ((ch = fgetc(input)) != EOF && ch != '\n'); // For support of comments after ;
        if (ch == '/') {
            int next_ch = fgetc(input);
            if (next_ch == '/') {
                while ((ch = fgetc(input)) != EOF && ch != '\n');
            } else if (next_ch == '*') {
                while ((ch = fgetc(input)) != EOF) {
                   if (ch == '*') {
                        int end_ch = fgetc(input);
                        if (end_ch == '/') {
                            break; // End of block comment found
                        } else if (end_ch == EOF) {
                            return; // End of file reached unexpectedly
                        } else {
                            ungetc(end_ch, input); // Not end, keep going
                        }
                    }
                }
            } else {
                ungetc(next_ch, input); // while ((ch = fgetc(input)) != EOF) {
                ungetc(ch, input);	// int next_ch = fgetc(input);
                break;
            }
        } else if (ch == '"') { // Handle text inside quotes
            while ((ch = fgetc(input)) != EOF && ch != '"') { // Output the text inside quotes one character at a time
                fputc(ch, output);  // Write the character to output
                global_offset++;
            }
            if (ch == '"') {
                continue; // A closing quote is found, move to next
            }else printf("No closing \" found\n");
        } else {
            ungetc(ch, input); // We will use this char for text to compile
            break;
        }
    }
}

// Function to parse mnemonics and return corresponding machine code
int parse_mnemonic(const char *str, uint64_t *machine_code, size_t *size, FILE *output) {
/*
    // Example code to modify previous byte based on the mnemonic (Example: "JMP" modification)
    // ...the example is given primarily if you need to fill bits in previous bytes, if you are making a RISC processor.
    if (output != NULL && strcmp(str, "OR") == 0) { // Generic mnemonic, for all mnemonic_set's
        // Example: Modify the previously written byte (could be a jump address or opcode)
        fseek(output, -1, SEEK_CUR);  // Seek one byte back
        uint8_t previous_byte = fgetc(output);
	fseek(output, -1, SEEK_CUR);  // Seek one byte back, because fgetc(output) moves the file pointer.
        fputc(previous_byte, output); // Modify it (this is an example modification)
	*size = 0; // mnemonic has a default size of 1 byte. But there is support for other sizes if you only change in this function.
        return 1; // mnemonic match
    }
*/
    if(mnemonic_set == 0){ // This is an empty mnemonic set, if you want to make you own in a include file with #DEFINE statements in it.
        *size = 0;
	return 0;
    }else if (mnemonic_set == 1) { // Default mode 6502/6510/65816
        if      (strcmp(str, "LDA#") == 0)	*machine_code = 0xA9; // LDA immediate. Load  Accumulator.
        else if (strcmp(str, "LDA") == 0)	*machine_code = 0xAD; // LDA absolute.  Load  Accumulator.
        else if (strcmp(str, "STA") == 0)	*machine_code = 0x8D; // STA absolute.  Store Accumulator.
        else if (strcmp(str, "STA[X]") == 0)	*machine_code = 0x9D; // STA absolute + X.  Store Accumulator.
        else if (strcmp(str, "LDX") == 0)	*machine_code = 0xAE; // LDX absolute.
        else if (strcmp(str, "LDY") == 0)	*machine_code = 0xAC; // LDY absolute.
	else if (strcmp(str, "LDX#") == 0)	*machine_code = 0xA2; // LDX# immediate. Load X.
        else if (strcmp(str, "LDY#") == 0)	*machine_code = 0xA0; // LDY# immediate.
        else if (strcmp(str, "STX") == 0)	*machine_code = 0x8E; // STX absolute.  Store X.
        else if (strcmp(str, "STY") == 0)	*machine_code = 0x8C; // STY absolute.  Store Y.
        else if (strcmp(str, "BCS") == 0)	*machine_code = 0xB0; // BCS relative.  Branch on Carry Set.
        else if (strcmp(str, "BPL") == 0)	*machine_code = 0x10; // BPL relative.  Branch on Plus.
        else if (strcmp(str, "SEC") == 0)	*machine_code = 0x38; // SEC implied.  Set Carry.
        else if (strcmp(str, "ROR") == 0)	*machine_code = 0x6A; // ROR A.  
        //else if (strcmp(str, "ROR16") == 0)	*machine_code = 0x6E; // ROR absolute.
        else if (strcmp(str, "BCC") == 0)	*machine_code = 0x90; // BCC relative. Branch on Carry Clear.
        else if (strcmp(str, "RTS") == 0)	*machine_code = 0x60; // RTS implied.   Return From Subroutine
        else if (strcmp(str, "ADC#") == 0)	*machine_code = 0x69; // ADC immediate. Add to accumulator.
        else if (strcmp(str, "JMP") == 0)	*machine_code = 0x4C; // JMP absolute.  Jump to address.
        else if (strcmp(str, "SBC#") == 0)	*machine_code = 0xE9; // SBC immediate. Subtract from Accumulator.
        else if (strcmp(str, "AND#") == 0)	*machine_code = 0x29; // AND immediate. Bitwise AND of Accumulator.
        else if (strcmp(str, "ORA#") == 0)	*machine_code = 0x09; // ORA immediate. Bitwise OR of Accumulator.
        else if (strcmp(str, "EOR#") == 0)	*machine_code = 0x49; // EOR immediate. Bitwise XOR of Accumulator.
        else if (strcmp(str, "BEQ") == 0)	*machine_code = 0xF0; // BEQ relative address. Branch if EQual.
        else if (strcmp(str, "BNE") == 0)	*machine_code = 0xD0; // BNE relative address. Branch it Not Equal.
        else if (strcmp(str, "BMI") == 0)	*machine_code = 0x30; // BMI relative address. Branch if MInus.
        else if (strcmp(str, "PHA") == 0)	*machine_code = 0x48; // PHA implied. Push Accumulator.
        else if (strcmp(str, "PLA") == 0)	*machine_code = 0x68; // PLA implied. Pull Accumulator.
        else if (strcmp(str, "NOP") == 0)	*machine_code = 0xEA; // NOP implied. No Operation.
        else if (strcmp(str, "RTI") == 0)	*machine_code = 0x40; // RTI implied. Return from Interrupt.
        else if (strcmp(str, "ASL") == 0)	*machine_code = 0x0A; // ASL implied. Arithmetic Shift Left of accumulator.
        else if (strcmp(str, "LSR") == 0)	*machine_code = 0x4A; // LSR implied. Logical Shift Right of accumulator.
        else if (strcmp(str, "CLC") == 0)	*machine_code = 0x18; // CLC implied. Clear Carry.
 	else if (strcmp(str, "LDA[X]") == 0)	*machine_code = 0xBD; // LDA absolute + x. Load A.
  	else if (strcmp(str, "JSR") == 0)	*machine_code = 0x20; // JSR absolute. Jump Saving Return / Jump Sub Routine.
  	else if (strcmp(str, "INX") == 0)	*machine_code = 0xE8; // INX implied. Increment X.
  	else if (strcmp(str, "INY") == 0)	*machine_code = 0xC8; // INY implied. Increment X.
  	else if (strcmp(str, "DEX") == 0)	*machine_code = 0xCA; // DEX implied. Decrement X.
  	else if (strcmp(str, "DEY") == 0)	*machine_code = 0x88; // DEY implied. Decrement Y.
  	else if (strcmp(str, "CMP#") == 0)	*machine_code = 0xC9; // CMP# immediate.
  	else if (strcmp(str, "CMP") == 0)	*machine_code = 0xCD; // CMP absolute. Compare with A.
  	else if (strcmp(str, "BIT") == 0)	*machine_code = 0x2C; // BIT absolute.
  	else if (strcmp(str, "CPX#") == 0)	*machine_code = 0xE0; // CPX immediate. ComPare with X.
  	else if (strcmp(str, "SEI") == 0)	*machine_code = 0x78; // SEI implied. SEt Interrupt.
  	else if (strcmp(str, "CLI") == 0)	*machine_code = 0x58; // CLI implied. CLear Interrupt.
  	else if (strcmp(str, "INC") == 0)	*machine_code = 0xEE; // INC absolute. Increase value in address.
  	else if (strcmp(str, "DEC") == 0)	*machine_code = 0xCE; // DEC absolute. Increase value in address.
  	else if (strcmp(str, "INC[X]") == 0)	*machine_code = 0xFE; // INC absolute. Increase value in address.
  	else if (strcmp(str, "DEC[X]") == 0)	*machine_code = 0xDE; // DEC absolute. Increase value in address.
  	else if (strcmp(str, "TAX") == 0)	*machine_code = 0xAA; // TAX implied. Transfer A to X
  	else if (strcmp(str, "TAY") == 0)	*machine_code = 0xA8; // TAY implied. Transfer A to Y
  	else if (strcmp(str, "TXA") == 0)	*machine_code = 0x8A; // TXA implied. Transfer X to A
  	else if (strcmp(str, "TYA") == 0)	*machine_code = 0x98; // TYA implied. Transfer Y to A
  	else if (strcmp(str, "TSX") == 0)	*machine_code = 0xBA; // TSX implied. Transfer Stack Pointer to X
  	else if (strcmp(str, "TXS") == 0)	*machine_code = 0x9A; // TXS implied. Transfer X to Stack Pointer
	// Some non standard mnemonics...
        else if (strcmp(str, "GOTO") == 0)	*machine_code = 0x4C; // JMP absolute.  Jump to address.
  	else if (strcmp(str, "GOSUB") == 0)	*machine_code = 0x20; // JSR absolute. Jump Saving Return / Jump Sub Routine.
        else if (strcmp(str, "RETURN") == 0)	*machine_code = 0x60; // RTS implied.   Return From Subroutine
        else if (strcmp(str, "PUSH") == 0)	*machine_code = 0x48; // PHA implied. Push Accumulator.
        else if (strcmp(str, "POP") == 0)	*machine_code = 0x68; // PLA implied. Pull Accumulator.
        else if (strcmp(str, "ADD#") == 0)	*machine_code = 0x69; // ADC immediate. Add to accumulator.
        else if (strcmp(str, "SUB#") == 0)	*machine_code = 0xE9; // SBC immediate. Subtract from Accumulator.
	else return 0;
        
        *size = 1; // mnemonic has a default size of 1 byte. But there is support for other sizes if you only change in this function.
        return 1; // mnemonic match
    } else if (mnemonic_set == 2) { // M6800 mode. The mnemonics may not reflect the normal ones for 6800. But se it as an example on how to add a new mnemonics set fast.
        if (strcmp(str, "LDA#") == 0)		*machine_code = 0xA9; // LDA immediate
        else if (strcmp(str, "LDA") == 0)	*machine_code = 0xAD; // LDA absolute
        else if (strcmp(str, "STA") == 0)	*machine_code = 0x8D; // STA absolute
        else if (strcmp(str, "RTS") == 0)	*machine_code = 0x39; // RTS Return from subroutine
        else if (strcmp(str, "ADD#") == 0)	*machine_code = 0x8B; // RTS Return From Subroutine
        else if (strcmp(str, "SUB#") == 0)	*machine_code = 0x8F; // RTS Return From Subroutine
        else if (strcmp(str, "JMP") == 0)	*machine_code = 0x7E; // RTS Return From Subroutine
        else if (strcmp(str, "BNE") == 0)	*machine_code = 0xCD; // RTS Return From Subroutine
        else return 0;
        
        *size = 1; // mnemonic has a default size of 1 byte. But there is support for other sizes if you only change in this function.
        return 1; // mnemonic match
    } else if (mnemonic_set == 3) { // Z80 mode. The mnemonics may not reflect the normal ones for 6800. But se it as an example on how to add a new mnemonics set fast.
        if (strcmp(str, "LDA#") == 0)		*machine_code = 0xA9; // LDA immediate
        else if (strcmp(str, "LDA") == 0)	*machine_code = 0xAD; // LDA absolute
        else if (strcmp(str, "STA") == 0)	*machine_code = 0x8D; // STA absolute
        else if (strcmp(str, "RTS") == 0)	*machine_code = 0x39; // RTS Return from subroutine
        else if (strcmp(str, "ADD#") == 0)	*machine_code = 0x8B; // RTS Return From Subroutine
        else if (strcmp(str, "SUB#") == 0)	*machine_code = 0x8F; // RTS Return From Subroutine
        else if (strcmp(str, "JMP") == 0)	*machine_code = 0x7E; // RTS Return From Subroutine
        else if (strcmp(str, "BNE") == 0)	*machine_code = 0xCD; // RTS Return From Subroutine
        else return 0;
        
        *size = 1; // mnemonic has a default size of 1 byte. But there is support for other sizes if you only change in this function.
        return 1; // mnemonic match
    } else {
        return 0; // No matching mnemonic set
    }
}

//void parse_and_write(FILE *input, FILE *output, int little_endian,int label_bytes) {
void parse_and_write(FILE *input, FILE *output, int little_endian) {
    char token[50];
    int var_size = 1; // only for decimal numbers
    uint64_t prev_code = 0; // For something that can be used for compiling to RISC processors

    // For #DEFINE
    char substitute[50];
    int define = 0 ;

    while (1) {
        skip_whitespace_and_comments(input,output);
        if (fscanf(input, "%49s", token) == EOF) break; // Support for characters up to 50 chars long.

        size_t len = strlen(token);
        uint64_t buffer[8] = {0};
        size_t size = 0;
	
        if (token[len - 1] == ':') { // NEW label
            token[len - 1] = '\0';
	    if(find_label(token) == (uint64_t)-1) 
	    	add_label(token, global_offset,label_bytes); // om denna label redan finns, så läggs det inte till pga if-satsen.
	    else if(pars_round == 0) printf("label namned %s is already declared before\n",token);
            continue;
        }

        uint64_t label_offset = find_label(token);
	// If use of a already created label...
        if (label_offset != (uint64_t)-1) { // Matched label / use of a already created label. Prepare for output
	    
            size = bytes_label(token); // Nr. of bytes a label creates
	    if(size >8)printf("Label was found that will create a buffer overflow, label(%s): %zu\n", token, size);

	    for(int i = 0 ; i < size ; i++){
	        buffer[(size-1) - i] = (label_offset >> (8*i)) & 0xFF ;
	    }
	} else if ((define != 0) || (strcmp(token, "#DEFINE") == 0)) {
		if (define == 0) define++ ;
		else if(define == 1){
			define++ ;
			strcpy( substitute, token );
		}else{
			define = 0 ;
			add_label(substitute, (uint64_t)strtoll(token, NULL, 16),(len+1)>>1);	
		}

	// These only work on decimal numbers
	} else if(strcmp(token, "8bit" ) == 0){	var_size = 1; // Default
	} else if(strcmp(token, "16bit") == 0){ var_size = 2;
	} else if(strcmp(token, "24bit") == 0){ var_size = 3;
	} else if(strcmp(token, "32bit") == 0){ var_size = 4;
	} else if(strcmp(token, "48bit") == 0){ var_size = 5;
	} else if(strcmp(token, "56bit") == 0){ var_size = 6;
	} else if(strcmp(token, "64bit") == 0){ var_size = 7;
	} else if(prev_code != 0){
		// Last word was has be OR:ed with other data. Need a processor to use it for.
        } else if (parse_mnemonic(token, &buffer[0], &size, output )) {
	    switch(size){ // For RISC generation support. OP-code is a combo of things, not one thing as in old CISC 8-bit processors.
		case 0: prev_code = buffer[0];break;// store the value that was sent out.
		default: prev_code = 0 ; break;
	    }
        } else if (token[0] == '.') { // input a relative address. Hardcoded to generate 1 byte offset
		size = 1;
		buffer[0] = (signed char)(find_label(token+1) - global_offset -1 ) ; // -1 because the instruction before this gives it the wrong offset
	} else if (token[0] == '$') {
	    if(len == 1) printf("Found a $ sign with no number behind it\n");
	    len-- ; // because $ should not be included
	    label_bytes = (len+1)>>1 ; // Will change the number of bytes used for addesses.// (len+1)>>1 this formula is used because 000 -> 2,  00 -> 1,  0000 -> 2
            global_offset = (uint64_t)strtoll(token + 1, NULL, 16);
            continue;
	} else if (token[0] == '%') { // include another bin file
		if(len == 1) printf("Found a %% sign with no number behind it\n");
		FILE *file = fopen(token + 1, "rb");  // Open in read-binary mode
		if (file == NULL) {
			perror("Error opening binary file for inclusion\n");
			return ;
		}
		size = 1 ;
		uint64_t byte;
		while (fread(&byte, sizeof(byte), 1, file) == 1) {
			// Process the byte (for example, print it as hexadecimal)
			global_offset += write_bytes(output, &byte, size, little_endian); // funkar eftersom size är 1
		}
		if (ferror(file)) perror("Error reading file");
		fclose(file);
		size = 0 ; // So the normal generation doesn't create anything
	} else if (token[0] == '!') { // include another file
	    if(len == 1) printf("Found a ! sign with no filename behind it\n");
	    FILE *input = fopen(token + 1, "r");
	    if (input == NULL) {
		perror("Error opening include file");
		return ;
	    }
	    parse_and_write(input, output, little_endian); //parse_and_write(input, output, little_endian, label_bytes);
	    fclose(input);
	    size = 0 ; // So the normal generation doesn't create anything
	} else if (token[len -1] == '$'){ // Fill with zeros till new address/global_conter match
	   if(len == 1) printf("Found a $ sign with no number behind it\n");
	   token[len - 1] = '\0'; //
	   uint64_t endpoint = (uint64_t)strtoll(token, NULL, 16);
	   size = 1 ; // We are writing one byte at a time
	   while( endpoint > (global_offset)){
		global_offset += write_bytes(output, buffer, size, little_endian);
	   }
	   size = 0 ; // So the normal generation doesn't create anything
	} else if (token[0] == '+'){
		if(len == 1) printf("Found a + sign with no number behind it\n");
		size = var_size;
		if(size > 8) printf("size is set to big in... +decimal\n");
		for(int i = 0 ; i < size ; i++){// Nr. of bytes a label creates
		    buffer[(size-1) - i] = ((uint64_t)strtoull(token, NULL, 10) >> (8*i)) & 0xFF ;
		}
	} else if (token[0] == '-'){
		if(len == 1) printf("Found a - sign with no number behind it\n");
		size = var_size;
		if(size > 8) printf("size is set to big in... -decimal\n");

		for(int i = 0 ; i < size ; i++){// Nr. of bytes a label creates
		    buffer[(size-1) - i] = ((uint64_t)strtoull(token, NULL, 10) >> (8*i)) & 0xFF ;
		}
        } else { // Convert hex numbers in text to data
	    char *endptr = NULL ;
	    uint64_t result = strtoull(token, &endptr, 16);
	    
	    if (*endptr != '\0') { // Check if conversion was successful
		if(pars_again){
			printf("Error %s not recogniced to have a set value.\n ",token);
			printf("If it's a mnemonic, have you selected the right processor");
			printf("otherwise there is currently no support for it.\n ");
		}
		pars_again = 1; // Probably a label with forward reference. Parse again!
		size = label_bytes ; // Must generate the right amount of data so labels pointed forward that is not resolved, match length for second parsing.
	    }else{ // it's a hex number
	    	size = (len+1)>>1; // No. of bytes a label creates
		if(size > 8) printf("size is set to big(%zu) by token ->%s<- \n",len,token);

		for(int i = 0 ; i < size ; i++){ // Convert hex to data
			buffer[(size-1) - i] = (result >> (8*i)) & 0xFF ;
		}
	    }
        }

	if(size > 8) printf("program tries to write %zu bytes to the buffer when given a new offset\n", size);
	//printf("%lx %s\n",global_offset,token); // For debugging
        global_offset += write_bytes(output, buffer, size, little_endian);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s [-big|-little|-MOS6502|-M6800|-Z80] <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    int arg_index = 1;
    if (strcmp(argv[1], "-little") == 0) {
        little_endian = 1;
        arg_index++;
    } else if (strcmp(argv[1], "-big") == 0) {
        arg_index++;
    } else if (strcmp(argv[1], "-little32") == 0) {
        little_endian = 1;
	label_bytes = 4;
        arg_index++;
    } else if (strcmp(argv[1], "-big32") == 0) {
	label_bytes = 4;
        arg_index++;
    } else if (strcmp(argv[1], "-MOS6502") == 0) {
	mnemonic_set  = 1; // The default. Row not needed.
	little_endian = 1; //
	label_bytes = 2; // The default. Row not needed. 16bit address bus gives the number 2.
        arg_index++;
    } else if (strcmp(argv[1], "-M6800") == 0) {
	mnemonic_set  = 2;
	little_endian = 0; // The default. Row not needed.
	label_bytes = 2; // The default. Row not needed. 16bit address bus gives the number 2.
        arg_index++;
    } else if (strcmp(argv[1], "-Z80") == 0) {
	mnemonic_set  = 3; // The default. Row not needed.
	little_endian = 1; //
	label_bytes = 2; // The default. Row not needed. 16bit address bus gives the number 2.
        arg_index++;
    } else { 
	mnemonic_set  = 0;
	printf("Uses default settings: \n");
	printf("* No mnemonics, 16bit address for labels.\n"); 
	printf("* Big endian formating.\n\n");
    }

    if (argc - arg_index != 2) {
        fprintf(stderr, "Usage: %s [-big|-little|-MOS6502|-M6800|-Z80] <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[arg_index], "r");
    if (input == NULL) {
        perror("Error opening input file");
        return 1;
    }

    FILE *output = fopen(argv[arg_index + 1], "w+b"); //"r+b"); //FILE *output = fopen(argv[arg_index + 1], "wb");
    if (output == NULL) {
        perror("Error opening output file");
        fclose(input);
        return 1;
    }

    parse_and_write(input, output, little_endian);
    // parse_and_write(input, output, little_endian, label_bytes);

    if (fflush(output) != 0) {
        perror("Error flushing output file");
    }

    if(pars_again){ // Rewinding files for second round for use of labels declared later in text (forward referencing).
	    printf("Possible forward reference labels, starts a second parsing\n");
	    fseek(input,  0, SEEK_SET);
	    fseek(output, 0, SEEK_SET);
	    pars_round++;
	    global_offset = 0;
	    parse_and_write(input, output, little_endian);
	    //parse_and_write(input, output, little_endian, label_bytes);
    }

    // Output the file size after writing
    fseek(output, 0, SEEK_END);
    long file_size = ftell(output);
    printf("Conversion complete. File size: %ld bytes\n", file_size); // Output the file size

    fclose(input);
    fclose(output);

    // Print out the generated label table
    printf("This is the generated label table:\n");
    for(int i = 0 ; i < label_count; i++){
	printf("%x bytes with the value 0x%lx named %s \n",labels[i].bytes,labels[i].offset,labels[i].name); // <------------------------------------------------------------------------------------------------------------------------------------------------------------
    }

    return 0;
}