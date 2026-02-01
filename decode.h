#ifndef DECODE_H
#define DECODE_H

#include "types.h"  // Include common types such as Status enum

#define MAGIC_STRING "#*"  // Magic string to identify stego data presence (must match encoding)

typedef struct _DecodeInfo
{
    /* Source Stego Image */
    char *stego_image_fname;   // Filename of the stego image input file
    FILE *fptr_stego_image;   // File pointer to the opened stego image

    /* Output File */
    char output_fname[100];   // Buffer for output secret file name (without extension)
    FILE *fptr_output;        // File pointer to the output decoded secret file

    /* Data extracted */
    char extn_secret_file[8]; // Buffer for the decoded secret file extension
    long size_secret_file;    // Size of the decoded secret file in bytes
} DecodeInfo;

/* Function Prototypes */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);  // Validate cmdline args and setup decode info

Status open_decode_files(DecodeInfo *decInfo);  // Open stego image file for reading

Status do_decoding(DecodeInfo *decInfo);  // Main orchestrator to perform all decode steps

Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo);  // Decode and verify magic string from image

Status decode_secret_file_extn_size(DecodeInfo *decInfo, int *size);   // Decode secret extension size from image

Status decode_secret_file_extn(DecodeInfo *decInfo, int size);         // Decode secret file extension string

Status decode_secret_file_size(DecodeInfo *decInfo, long *size);       // Decode secret file size from image

Status decode_secret_file_data(DecodeInfo *decInfo, long size);        // Decode the actual secret file data bytes

char decode_byte_from_lsb(char *image_buffer);  // Decode a byte value from LSBs of 8 given bytes

int decode_size_from_lsb(char *image_buffer);   // Decode a 32-bit integer from LSBs of 32 given bytes

#endif
