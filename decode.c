#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    printf(YELLOW"Read and validate decode arguments started...\n"RESET);

    if (argv[2] == NULL) // Check if stego image arg is missing
    {
        printf(RED"INVALID: Missing stego image file.\n"RESET);
        return e_failure;
    }

    if (strstr(argv[2], ".bmp") == NULL) // Check file extension is .bmp
    {
        printf(RED"INVALID: Stego image must have a .bmp extension.\n"RESET);
        return e_failure;
    }

    decInfo->stego_image_fname = argv[2]; // Store stego image filename

    if (argv[3] != NULL) // If output filename is specified
    {
        static char outputBuffer[100]; // Buffer to hold output filename
        strcpy(outputBuffer, argv[3]); // Copy filename argument to buffer

        char *dot = strrchr(outputBuffer, '.'); // Find file extension dot

        if (dot == NULL) // If no dot found
        {
            printf(RED"INVALID: The output file must have a valid extension (.txt, .c, or .sh)\n"RESET);
            return e_failure;
        }

        // Check for allowed extensions
        if (strcmp(dot, ".txt") == 0 || strcmp(dot, ".c") == 0 || strcmp(dot, ".sh") == 0)
        {
            *dot = '\0'; // Remove extension from buffer
            strcpy(decInfo->output_fname, outputBuffer); // Store base filename
            printf(BLUE"Valid output file extension detected: %s\n"RESET, dot + 1); // Print extension
            printf(BLUE"Output base filename set to '%s'\n"RESET, decInfo->output_fname); // Print base name
        }
        else
        {
            printf(RED"INVALID: Unsupported file extension '%s'.\n"RESET, dot);
            printf(RED"Please provide a file with one of the following extensions: .txt, .c, or .sh\n"RESET);
            return e_failure;
        }
    }
    else // No output filename provided
    {
        printf(RED"Error:No output filename provided. Using default: 'decoded'\n"RESET);
        strcpy(decInfo->output_fname, "decoded"); // Use default output name
    }

    return e_success; // Args validated successfully
}

Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb"); // Open stego image binary
    if (decInfo->fptr_stego_image == NULL) // Check fopen success
    {
        perror("fopen"); // Print system error message
        fprintf(stderr, RED"ERROR: Unable to open file %s\n"RESET, decInfo->stego_image_fname); // Print custom error
        return e_failure;
    }

    printf(BLUE"Stego image opened successfully: %s\n"RESET, decInfo->stego_image_fname); // Print success
    return e_success;
}

Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo)
{
    printf(YELLOW"Decoding magic string...\n"RESET);
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET); // Skip BMP header by seeking to byte 54

    char image_buffer[8]; // Buffer to read 8 bytes per decoded char
    char decoded_char;
    char decoded_magic[10] = {0}; // Buffer to store decoded magic string

    for (int i = 0; i < strlen(magic_string); i++) // Loop over magic string length
    {
        fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image); // Read next 8 bytes
        decoded_char = decode_byte_from_lsb(image_buffer); // Decode a byte from LSBs
        decoded_magic[i] = decoded_char; // Store decoded byte in buffer
    }

    decoded_magic[strlen(magic_string)] = '\0'; // Null-terminate decoded magic string

    if (strcmp(decoded_magic, magic_string) == 0) // Compare decoded with expected magic string
    {
        printf(BLUE"Magic string verified successfully: %s\n"RESET, decoded_magic);
        return e_success; // Match success
    }
    else
    {
        printf(RED"Erorr: Magic string mismatch. Not a valid stego image.\n"RESET);
        return e_failure; // Match failure
    }
}

Status decode_secret_file_extn_size(DecodeInfo *decInfo, int *size)
{
    printf(YELLOW"Decoding secret file extn size...\n"RESET);
    char image_buffer[32]; // Buffer to read 32 bytes for extension size
    fread(image_buffer, sizeof(char), 32, decInfo->fptr_stego_image);
    *size = decode_size_from_lsb(image_buffer); // Decode size from LSBs
    printf(BLUE"Decoded secret file extension size = %d\n"RESET, *size); // Print decoded size
    return e_success;
}

Status decode_secret_file_extn(DecodeInfo *decInfo, int size)
{
    printf(YELLOW"Decoding secret file extn...\n"RESET);
    char image_buffer[8]; // Buffer to read 8 bytes at a time
    for (int i = 0; i < size; i++) // Loop for extension size bytes
    {
        fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image); // Read 8 bytes
        decInfo->extn_secret_file[i] = decode_byte_from_lsb(image_buffer); // Decode and store
    }
    decInfo->extn_secret_file[size] = '\0'; // Null-terminate extension string
    printf(BLUE"Decoded extension = %s\n"RESET, decInfo->extn_secret_file); // Print decoded extension

    strcat(decInfo->output_fname, decInfo->extn_secret_file); // Append extension to output filename

    decInfo->fptr_output = fopen(decInfo->output_fname, "w"); // Open output file for writing
    if (decInfo->fptr_output == NULL) // Check fopen success
    {
        perror("fopen"); // Print system error
        fprintf(stderr, RED"INVALID:: Unable to open output file %s\n"RESET, decInfo->output_fname);
        return e_failure;
    }

    printf(BLUE"Output file created: %s\n"RESET, decInfo->output_fname); // Print output filename
    return e_success;
}

Status decode_secret_file_size(DecodeInfo *decInfo, long *size)
{
    printf(YELLOW"Decoding secret file size...\n"RESET);
    char image_buffer[32]; // Buffer for 32 bytes
    fread(image_buffer, sizeof(char), 32, decInfo->fptr_stego_image);
    *size = decode_size_from_lsb(image_buffer); // Decode file size from LSB
    decInfo->size_secret_file = *size; // Store size in decode info
    printf(BLUE"Decoded secret file size = %ld bytes\n"RESET, *size); // Print size
    return e_success;
}

Status decode_secret_file_data(DecodeInfo *decInfo, long size)
{
    char image_buffer[8]; // Buffer for 8 bytes
    char ch;

    printf(YELLOW"Decoding secret file data...\n"RESET);
    for (long i = 0; i < size; i++) // Loop for every byte of secret
    {
        fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image); // Read 8 bytes
        ch = decode_byte_from_lsb(image_buffer); // Decode byte from LSB
        fputc(ch, decInfo->fptr_output); // Write byte to output file
    }
    printf(BLUE"Decoded secret data successfully.\n"RESET);
    return e_success;
}

char decode_byte_from_lsb(char *image_buffer)
{
    char data = 0;
    for (int i = 0; i < 8; i++) // Extract LSB bit by bit from 8 bytes
        data = (data << 1) | (image_buffer[i] & 1);
    return data; // Return decoded byte
}

int decode_size_from_lsb(char *image_buffer)
{
    int size = 0;
    for (int i = 0; i < 32; i++) // Extract 32 bits LSB for integer
        size = (size << 1) | (image_buffer[i] & 1);
    return size; // Return decoded integer
}

Status do_decoding(DecodeInfo *decInfo)
{
    printf(GREEN "------------ DECODING MODE ------------\n" RESET);

    if (open_decode_files(decInfo) == e_failure) // Open stego image
    {
        printf(RED "ERROR: Unable to open stego image file.\n" RESET);
        return e_failure;
    }

    if (decode_magic_string(MAGIC_STRING, decInfo) == e_failure) // Verify magic string
    {
        printf(RED "ERROR: Magic string mismatch. Not a valid stego image.\n" RESET);
        return e_failure;
    }

    int extn_size = 0;
    if (decode_secret_file_extn_size(decInfo, &extn_size) == e_failure) // Decode extension size
    {
        printf(RED "ERROR: Failed to decode secret file extension size.\n" RESET);
        return e_failure;
    }

    if (decode_secret_file_extn(decInfo, extn_size) == e_failure) // Decode extension string
    {
        printf(RED "ERROR: Failed to decode secret file extension.\n" RESET);
        return e_failure;
    }

    long secret_size = 0;
    if (decode_secret_file_size(decInfo, &secret_size) == e_failure) // Decode secret file size
    {
        printf(RED "ERROR: Failed to decode secret file size.\n" RESET);
        return e_failure;
    }

    if (decode_secret_file_data(decInfo, secret_size) == e_failure) // Decode secret file data
    {
        printf(RED "ERROR: Failed to decode secret file data.\n" RESET);
        return e_failure;
    }

    return e_success;
}


