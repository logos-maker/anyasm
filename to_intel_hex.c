// Converts a binary file to intel hex


#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 16

void print_hex_record(int address, unsigned char *data, size_t length) {
    // Start Code
    printf(":");
    
    // Length of Data
    printf("%02x", (unsigned char) length);
    
    // Address
    printf("%04x", address);
    
    // Record Type (00 = data record)
    printf("00");
    
    // Data
    for (size_t i = 0; i < length; i++) {
        printf("%02x", data[i]);
    }
    
    // Checksum Calculation
    int checksum = length + (address >> 8) + (address & 0xFF) + 0; // record type is 00
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    checksum = (0xFF - (checksum & 0xFF)) & 0xFF;

    // Checksum
    printf("%02x\n", (unsigned char) checksum);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *bin_file = fopen(argv[1], "rb");
    if (!bin_file) {
        perror("Unable to open binary file");
        return EXIT_FAILURE;
    }

    unsigned char buffer[BUFFER_SIZE];
    int address = 0;
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, bin_file)) > 0) {
        print_hex_record(address, buffer, bytes_read);
        address += bytes_read;
    }

    // End Of File record
    printf(":00000001FF\n");

    fclose(bin_file);
    return EXIT_SUCCESS;
}