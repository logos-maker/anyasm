#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_hex(FILE *input, FILE *output, int clean_output, int semicolon) {
    unsigned char buffer[16];  // Buffer for 16 bytes (one line)
    int bytes_read;
    int offset = 0;
    int i;

    // Read file in chunks of 16 bytes
    while ((bytes_read = fread(buffer, 1, 16, input)) > 0) {
        // Print hex values
        for (i = 0; i < 16; i++) {
            if (i < bytes_read) {
                fprintf(output, "%02X ", buffer[i]);
            } else {
                fprintf(output, "   ");  // Space padding for partial lines
            }
            
            // Add space between groups of 4 bytes
            if ((i + 1) % 4 == 0 && i < 15) {
                fprintf(output, " ");
            }
        }
        
        // Print address at end of line (if not clean output)
        if (!clean_output) {
            if (semicolon) {
                fprintf(output, "; %08X", offset);
            } else {
                fprintf(output, " // %08X", offset);
            }
        }
        
        fprintf(output, "\n");
        offset += 16;
    }
}

int main(int argc, char *argv[]) {
    FILE *input_file, *output_file;
    int clean_output = 0;
    int semicolon = 0;
    int arg_offset = 0;

    // Check command line arguments
    if (argc < 3 || argc > 4) {
        printf("Usage: %s [-c|-s] <input_binary_file> <output_text_file>\n", argv[0]);
        printf("  -c: Clean output (no address comments)\n");
        printf("  -s: Use semicolon instead of // for addresses\n");
	printf("Example> ./bintotext -c hello.prg hello.hex\n");
        return 1;
    }

    // Check for flags
    if (argc == 4) {
        if (strcmp(argv[1], "-c") == 0) {
            clean_output = 1;
            arg_offset = 1;
        } else if (strcmp(argv[1], "-s") == 0) {
            semicolon = 1;
            arg_offset = 1;
        } else {
            printf("Unknown option: %s\n", argv[1]);
            printf("Use -c for clean output or -s for semicolon comments\n");
            return 1;
        }
    }

    // Open input file in binary mode
    input_file = fopen(argv[1 + arg_offset], "rb");
    if (input_file == NULL) {
        printf("Error: Could not open input file %s\n", argv[1 + arg_offset]);
        return 1;
    }

    // Open output file in text mode
    output_file = fopen(argv[2 + arg_offset], "w");
    if (output_file == NULL) {
        printf("Error: Could not open output file %s\n", argv[2 + arg_offset]);
        fclose(input_file);
        return 1;
    }

    // Convert binary to hex text
    print_hex(input_file, output_file, clean_output, semicolon);

    // Clean up
    fclose(input_file);
    fclose(output_file);

    printf("Conversion complete!\n");
    return 0;
}