/*
*Name:DEVARA SIVANANDA KUMAR
*Date:11/11/25
*Project Title:LSB IMAGE STEGANOGRAPHY
*Description: 
This project is a console-based LSB Image Steganography application built for Linux terminal environments. 
It enables users to hide and retrieve secret data—such as text or files—inside BMP images using the 
Least Significant Bit (LSB) technique, a widely used and effective method of steganography.

Featuring a menu-driven command-line interface, the program provides an intuitive workflow for performing encoding, 
decoding, and verification tasks. All operations preserve the visual quality of the image, 
ensuring that the embedded message remains invisible to the human eye.

This project demonstrates practical knowledge of bit-level manipulation, BMP image structure, and file I/O operations in C. 
It serves as a strong learning experience in image processing, binary operations, and foundational information security techniques.

*In Encode:
Validation and setup: It verifies the validity of input BMP and secret files (only .txt, .c, or .sh are allowed for secrets) 
and opens source, secret, and output BMP files with robust error handling.
Capacity check: Determines if the source BMP image has enough unused pixel data to store the secret file, 
including metadata.
Encoding process: Copies the original BMP header to the output, encodes a 'magic string' (hidden identifier), 
then sequentially embeds metadata (such as secret file extension, sizes) and secret file data using least-significant-bit (LSB) manipulation. Each byte of secret information is split over 8 BMP image bytes, minimizing any visible changes.
Finalization: Any remaining image data is copied to the output image to maintain original image integrity.

*In Decode:
Validation and setup: It confirms the input file is a BMP, ensures proper output file extension, 
and manages file pointers with error feedback.
Decoding process: Skips the BMP header, verifies the magic string to check for a valid stego image, 
decodes the secret file's extension and size (embedded as metadata), and reconstructs the secret file by extracting bits from LSBs of the image bytes. It writes the recovered secret data to the specified output file.
Error handling and messaging: The file provides detailed feedback on validation results, decoding progress, 
and respective errors to help diagnose issues quickly.

Each file is carefully organized to ensure modularity, robust parameter validation, and comprehensive error handling, 
making it suitable for practical steganography applications using BMP images.
*/
#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

// Function to determine operation mode from command line argument
OperationType check_operation_type(char *symbol);

int main(int argc, char *argv[])
{
    // Check if the minimum number of arguments is provided
    if(argc < 3)
    {
        printf(RED"  Enter valid Argument:\n"RESET);
        printf(RED"  To Encode : ./a.out -e <source.bmp> <secret.txt> [output.bmp]\n"RESET);
        printf(RED"  To Decode : ./a.out -d <stego.bmp> [output.txt]\n"RESET);
        return e_failure;
    }

    // Determine operation type: encode or decode
    OperationType operation = check_operation_type(argv[1]);

    if (operation == e_encode)
    {
        // Encoding requires 4 or 5 arguments total
        if (argc < 4 || argc > 5)
        {
            printf(RED"Error: Invalid number of arguments for encoding.\n"RESET);
            printf(RED"Usage: ./a.out -e <source.bmp> <secret.txt> [output.bmp]\n"RESET);
            return e_failure;
        }

        printf(GREEN"\n------------ ENCODING MODE ------------\n"RESET);
        EncodeInfo encInfo;

        // Validate encoding arguments
        if(read_and_validate_encode_args(argv, &encInfo) == e_success)
        {
            printf(BLUE"Validation successful.\n"RESET);
            // Call encoding function
            if (do_encoding(&encInfo) == e_success)
                printf(BLUE"Encoding Completed Successfully.\n"RESET);
            else
                printf(RED"Encoding Failed.\n"RESET);
        }
        else
        {
            printf(RED"Validation Failed.\n"RESET);
            return e_failure;
        }
    }
    else if(operation == e_decode)
    {
        // Decoding accepts 3 or 4 total arguments
        if (argc < 3 || argc > 4)
        {
            printf(RED"Error: Invalid number of arguments for decoding.\n"RESET);
            printf(RED"Usage: ./a.out -d <stego.bmp> [output.txt]\n"RESET);
            return e_failure;
        }

        printf(GREEN"\n------------ DECODING MODE ------------\n"RESET);
        DecodeInfo decInfo;

        // Validate decoding arguments
        if(read_and_validate_decode_args(argv, &decInfo) == e_success)
        {
            printf(BLUE"Validation successful.\n"RESET);
            // Call decoding function
            if(do_decoding(&decInfo) == e_success)
            {
                printf(BLUE"Decoding Completed Successfully.\n"RESET);
            }
            else
            {
                printf(RED"Decoding Failed.\n"RESET);
            }
        }
        else
        {
            printf(RED"Validation Failed.\n"RESET);
            return e_failure;
        }
    }
    else
    {
        // Unsupported operation argument
        printf(RED"Unsupported Operation! Use:\n"RESET);
        printf(RED"  -e for Encoding\n"RESET);
        printf(RED"  -d for Decoding\n"RESET);
        return e_failure;
    }

    // Program completed successfully
    return e_success;
}

// Determine operation type enum from user input
OperationType check_operation_type(char *symbol)
{
    if(strcmp(symbol, "-e") == 0)
    {
        return e_encode; // Encoding mode selected
    }
    else if (strcmp(symbol, "-d") == 0)
    {
        return e_decode; // Decoding mode selected
    }
    else
    {
        return e_unsupported; // Unsupported or invalid option
    }
}
