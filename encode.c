#include <stdio.h>
#include "encode.h"
#include "types.h"
#include <string.h>
#include "common.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);
    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    // Return image capacity
    return width * height * 3;
}
// Find the size of secret file data
uint get_file_size(FILE *fptr)
{
    
    fseek(fptr, 0, SEEK_END);      // Move to end of file
    uint size = ftell(fptr);       // Get current file position (end = size)
    rewind(fptr);                  // Reset to start
    return size;
}



/*
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
 
    // Validate source file
    if(strstr(argv[2], ".bmp") != NULL) 
    {
        encInfo -> src_image_fname = argv[2];
    }
    else 
    {
        printf(RED"Invalid : source file must be a .bmp files\n"RESET);
        return e_failure;
    }

    char *dot = strrchr(argv[3], '.');
    if(dot == NULL)
    {
        printf(RED"Invalid :secret file must be a .txt .c and .sh  files are allowed\n"RESET);
        return e_failure;   
    }
    // Validate secret message file
    if(strcmp(dot, ".txt") == 0)
    {
        encInfo -> secret_fname = argv[3];
    } 
    else if(strcmp(dot, ".c")== 0)
    {
        encInfo -> secret_fname = argv[3];
    }
    else if(strcmp(dot, ".sh")== 0)
    {
        encInfo -> secret_fname = argv[3];
    } 
    else 
    {
        printf(RED"Invalid :secret file must be a .txt .c and .sh  files are allowed\n"RESET);
        return e_failure;
    }
    //validate output bmp file
    if(argv[4] == NULL)
    {
        encInfo -> stego_image_fname = "default.bmp";
    }
    else if(strstr(argv[4], ".bmp")==NULL)
    {
        printf(RED"INVALID: output file must be a .bmp file\n"RESET);
        return e_failure;
    }
    else
    {
        encInfo -> stego_image_fname = argv[4];
    }

    return e_success;
}
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if(encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "INVALID: Unable to open file %s\n", encInfo->src_image_fname);
        return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "INVALID: Unable to open file %s\n", encInfo->secret_fname);
        return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "INVALID: Unable to open file %s\n", encInfo->stego_image_fname);
        return e_failure;
    }

    // No failure return e_success
    return e_success;
}

Status  check_capacity(EncodeInfo *encInfo)
{
    printf(YELLOW"Checking image capacity...\n"RESET);                      // Print action start
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image); // Get BMP image pixel data capacity
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);          // Get size of secret file

    int capacity = 16 + 32 + 32 + 32 + (encInfo->size_secret_file * 8);       // Calculate required capacity including magic string and metadata

    if (encInfo->image_capacity > capacity)                    // Compare capacity against requirement
    {
        printf(BLUE"Image has sufficient capacity: %u bytes available.\n"RESET, encInfo->image_capacity); // Print success info
        return e_success;                                       // Indicate success
    }
    else
    {
        printf(RED"Error: Image capacity (%u bytes) insufficient for encoding.\n"RESET, encInfo->image_capacity); // Print failure info
        return e_failure;                                       // Indicate failure
    }
}

Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    printf(YELLOW"Copying BMP header...\n"RESET);                         // Print action
    rewind(fptr_src_image);                                    // Seek source file pointer to file start
    char header[54];                                           // Declare buffer for BMP header bytes
    fread(header, 54, 1, fptr_src_image);                      // Read 54-byte header from source
    fwrite(header, 54, 1, fptr_dest_image);                    // Write 54-byte header to destination
    if(ftell(fptr_src_image) == ftell(fptr_dest_image))        // Compare file pointers to check accuracy
    {
        return e_success;                                       // Return success if match
    }
    else
    {
        fprintf(stderr, RED"Error: BMP header copy failed.\n"RESET);  // Report error and fail otherwise
        return e_failure;
    }    
}

Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)                                // Loop bitwise over 8 bits of data byte
    {
        image_buffer[i] &= 0xFE;                               // Clear the least significant bit of image byte
        image_buffer[i] |= ((data >> (7 - i)) & 1);            // Write data bit into LSB of image byte
    }
    return e_success;                                           // Success indication
}

Status encode_size_to_lsb(int size, char *imageBuffer)
{
    for (int i = 0; i < 32; i++)                               // Loop bitwise over 32 bits of size integer
    {
        imageBuffer[i] &= 0xFE;                                // Clear LSB of image byte
        imageBuffer[i] |= ((size >> (31 - i)) & 1);            // Write bit of size into LSB
    }
    return e_success;                                           // Success indication
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    printf(YELLOW"Encoding magic string...\n"RESET);                      // Print action started
    char imageBuffer[8];                                        // Buffer holds 8 bytes to encode one char bitwise

    for (int i = 0; i < (int)strlen(magic_string); i++)        // Loop over each character of magic string
    {
        if(fread(imageBuffer, 8, 1, encInfo->fptr_src_image) != 1)     // Read 8 bytes from source image
        {
            fprintf(stderr, RED"Error: Reading source image buffer failed at magic string encoding.\n"RESET);
            return e_failure;
        }
        encode_byte_to_lsb(magic_string[i], imageBuffer);     // Encode one char to the 8 bytes' LSB
        if(fwrite(imageBuffer, 8, 1, encInfo->fptr_stego_image) != 1)  // Write modified bytes to stego image
        {
            fprintf(stderr, RED"Error: Writing stego image buffer failed at magic string encoding.\n"RESET);
            return e_failure;
        }
    }
    return e_success;                                           // Indicate success for magic string encode
}

// The remaining functions follow the same commenting style

Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    printf(YELLOW"Encoding secret file extension size..\n"RESET);
    char imageBuffer[32];
    if(fread(imageBuffer, 32, 1, encInfo->fptr_src_image) != 1)    // Read 32 bytes from source image
    {
        fprintf(stderr, RED"Error: Reading source image buffer failed at extension size encoding.\n"RESET);
        return e_failure;
    }
    encode_size_to_lsb(size, imageBuffer);                       // Encode size in LSBs of this buffer
    if(fwrite(imageBuffer, 32, 1, encInfo->fptr_stego_image) != 1)  // Write encoded buffer to stego image
    {
        fprintf(stderr, RED"Error: Writing stego image buffer failed at extension size encoding.\n"RESET);
        return e_failure;
    }
    return e_success;
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    printf(YELLOW"Encoding secret file extension...\n"RESET);
    char imageBuffer[8];

    for(int i = 0; i < (int)strlen(file_extn); i++)             // For each char in extension
    {
        if(fread(imageBuffer, 8, 1, encInfo->fptr_src_image) != 1)  // Read 8 source bytes
        {
            fprintf(stderr, RED"Error: Reading source image buffer failed at file extension encoding.\n"RESET);
            return e_failure;
        }
        encode_byte_to_lsb(file_extn[i], imageBuffer);          // Encode char bits in LSBs
        if(fwrite(imageBuffer, 8, 1, encInfo->fptr_stego_image) != 1)  // Write to stego
        {
            fprintf(stderr, RED"Error: Writing stego image buffer failed at file extension encoding.\n"RESET);
            return e_failure;
        }
    }
    return e_success;
}

Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    printf(YELLOW"Encoding secret file size...\n"RESET);
    char imageBuffer[32];
    if(fread(imageBuffer, 32, 1, encInfo->fptr_src_image) != 1)    // Read 32 bytes from source image
    {
        fprintf(stderr, RED"Error: Reading source image buffer failed at file size encoding.\n"RED);
        return e_failure;
    }
    encode_size_to_lsb(file_size, imageBuffer);                    // Encode file size LSBs
    if(fwrite(imageBuffer, 32, 1, encInfo->fptr_stego_image) != 1)  // Write encoded buffer to stego
    {
        fprintf(stderr, RED"Error: Writing stego image buffer failed at file size encoding.\n"RESET);
        return e_failure;
    }
    return e_success;
}

Status encode_secret_file_data(EncodeInfo *encInfo)
{
    printf(YELLOW"Encoding secret file data...\n"RESET);
    if(!encInfo || !encInfo->fptr_secret || !encInfo->fptr_src_image || !encInfo->fptr_stego_image)
    {
        fprintf(stderr, RED"Error: Invalid file pointers detected for encoding secret data.\n"RESET);
        return e_failure;
    }
    rewind(encInfo->fptr_secret);                              // Start reading secret file at beginning

    int byte_read;
    char imageBuffer[8];

    while(fread(&byte_read, 1, 1, encInfo->fptr_secret) == 1)   // Read each byte from secret file
    {
        if(fread(imageBuffer, 8, 1, encInfo->fptr_src_image) != 1)  // Read 8 source bytes
        {
            fprintf(stderr, RED"Error: Reading source image buffer failed during secret data encoding.\n"RESET);
            return e_failure; 
        }
        encode_byte_to_lsb((char)byte_read, imageBuffer);       // Encode byte in LSBs
        if(fwrite(imageBuffer, 8, 1, encInfo->fptr_stego_image) != 1)  // Write to stego image
        {
            fprintf(stderr, RED"Error: Writing stego image buffer failed during secret data encoding.\n"RESET);
            return e_failure; 
        }
    }
    return e_success;
}

Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    printf(YELLOW"Copying remaining image data...\n"RESET);
    char ch;
    while(fread(&ch, 1, 1, fptr_src) > 0)                       // Read byte-by-byte to end of source image
    {
        if(fwrite(&ch, 1, 1, fptr_dest) !=1 )                   // Write each byte to destination
        {
            fprintf(stderr, RED"Error: Writing remaining image data failed.\n"RESET);
            return e_failure;
        }
    }
    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    if(open_files(encInfo) == e_failure)      // Open all files, check for errors
    {
        fprintf(stderr, RED"Error: Opening files failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Files opened successfully.\n"RESET);

    if(check_capacity(encInfo) == e_failure)  // Check capacity to hold secret data
    {
        fprintf(stderr, RED"Error: Image capacity insufficient for secret data.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Capacity check passed.\n"RESET);

    if(copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure) // Copy BMP header
    {
        fprintf(stderr, RED"Error: Copying BMP header failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"BMP header copied successfully.\n"RESET);

    if(encode_magic_string(MAGIC_STRING, encInfo) == e_failure) // Encode magic string
    {
        fprintf(stderr, RED"Error: Encoding magic string failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Magic string encoded successfully.\n"RESET);

    char *extn = strstr(encInfo->secret_fname, ".");           // Get secret file extension
    int extn_size = strlen(extn);

    if(encode_secret_file_extn_size(extn_size, encInfo) == e_failure)  //Encode extension size
    {
        fprintf(stderr, RED"Error: Encoding secret file extension size failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Secret file extension size encoded successfully.\n"RED);

    if(encode_secret_file_extn(extn, encInfo) == e_failure)     // Encode extension string
    {
        fprintf(stderr, RED"Error: Encoding secret file extension failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Secret file extension encoded successfully.\n"RESET);

    if(encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure) // Encode file size
    {
        fprintf(stderr, RED"Error: Encoding secret file size failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Secret file size encoded successfully.\n"RESET);

    if(encode_secret_file_data(encInfo) == e_failure)          // Encode secret file data
    {
        fprintf(stderr, RED"Error: Encoding secret file data failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Secret file data encoded successfully.\n"RESET);

    if(copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure) // Copy remaining image data
    {
        fprintf(stderr, RED"Error: Copying remaining image data failed.\n"RESET);
        return e_failure;
    }
    printf(BLUE"Remaining image data copied successfully.\n"RESET);

    printf(BLUE"Encoding complete! Stego image saved as %s\n"RESET, encInfo->stego_image_fname); // Final success message
    return e_success;
}
