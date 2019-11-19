/* * CSCI3280 Introduction to Multimedia Systems 
* 
* --- Declaration --- 
* 
* I declare that the assignment here submitted is original except for source 
* material explicitly acknowledged. I also acknowledge that I am aware of 
* University policy and regulations on honesty in academic work, and of the 
* disciplinary guidelines and procedures applicable to breaches of such policy 
* and regulations, as contained in the website 
* http://www.cuhk.edu.hk/policy/academichonesty/ 
* 
* Assignment 2 
* Name : Ajay Singh Ramsingh Raghuwanshi
* Student ID : 1155083332
* Email Addr : ajaygrader@gmail.com/1155083332@link.cuhk.edu.hk
*/
#include"stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define CODE_SIZE  13
#define TRUE 1
#define FALSE 0
#define SIZEOFHASHTABLE 9029 
#define MAXIMUMVALUE (1 << CODE_SIZE) - 1 
#define HASHSHIFT (CODE_SIZE-8)   
#define MAX_CODE MAXIMUMVALUE - 1    

/* function prototypes */
unsigned int read_code(FILE*, unsigned int);
void write_code(FILE*, unsigned int, unsigned int);
void writefileheader(FILE *, char**, int);
void readfileheader(FILE *, char**, int *);
void compress(FILE*, FILE*);
void decompress(FILE*, FILE*);
unsigned char *decode(unsigned char*, unsigned int);
int match(int , unsigned int);

unsigned char decoded_string[4000], *char_append; // array holding the decoded string and appended chars 
unsigned int *prefix;        //prefix codes   
int *codeval;                 //code value array  

int main(int argc, char **argv)
{
	codeval = (int*)malloc(SIZEOFHASHTABLE * sizeof(int));
	char_append = (unsigned char *)malloc(SIZEOFHASHTABLE * sizeof(unsigned char));
	int printusage = 0;
	int	no_of_file;
	char **input_file_names;
	char *output_file_names;
	FILE *input_file, *output_file, *lzw_file;
	prefix = (unsigned int *)malloc(SIZEOFHASHTABLE * sizeof(unsigned int));

	if (argc >= 3)
	{
		if (strcmp(argv[1], "-c") == 0)
		{
			/* compression */
			input_file = fopen(argv[3], "rb");//I could not implement archiving thus manually open a single file
			lzw_file = fopen(argv[2], "wb");
			/* write the file header */
			input_file_names = argv + 3;
			no_of_file = argc - 3;
			writefileheader(lzw_file, input_file_names, no_of_file);
			compress(input_file, lzw_file);
			/* ADD CODES HERE */
			fclose(input_file);
			fclose(lzw_file);
		}
		else
			if (strcmp(argv[1], "-d") == 0)
			{
				/* decompress */
				lzw_file = fopen(argv[2], "rb");
				output_file = fopen("test.txt", "wb");//manually changing format and name of single output file
				/* read the file header */
				no_of_file = 0;
				readfileheader(lzw_file, &output_file_names, &no_of_file);
				decompress(lzw_file, output_file);
				/* ADD CODES HERE */
				fclose(lzw_file);
				fclose(output_file);
				free(prefix);
				free(char_append);
				free(output_file_names);
			}
			else
				printusage = 1;
	}
	else
		printusage = 1;

	if (printusage)
		printf("Usage: %s -<c/d> <lzw filename> <list of files>\n", argv[0]);

	return 0;
}

/*****************************************************************
*
* writefileheader() -  write the lzw file header to support multiple files
*
****************************************************************/
void writefileheader(FILE *lzw_file, char** input_file_names, int no_of_files)
{
	int i;
	/* write the file header */
	for (i = 0; i < no_of_files; i++)
	{
		fprintf(lzw_file, "%s\n", input_file_names[i]);

	}
	fputc('\n', lzw_file);

}

/*****************************************************************
*
* readfileheader() - read the fileheader from the lzw file
*
****************************************************************/
void readfileheader(FILE *lzw_file, char** output_filenames, int * no_of_files)
{
	int noofchar;
	char c, lastc;

	noofchar = 0;
	lastc = 0;
	*no_of_files = 0;
	/* find where is the end of double newline */
	while ((c = fgetc(lzw_file)) != EOF)
	{
		noofchar++;
		if (c == '\n')
		{
			if (lastc == c)
				/* found double newline */
				break;
			(*no_of_files)++;
		}
		lastc = c;
	}

	if (c == EOF)
	{
		/* problem .... file may have corrupted*/
		*no_of_files = 0;
		return;

	}
	/* allocate memeory for the filenames */
	*output_filenames = (char *)malloc(sizeof(char)*noofchar);
	/* roll back to start */
	fseek(lzw_file, 0, SEEK_SET);

	fread((*output_filenames), 1, (size_t)noofchar, lzw_file);

	return;
}

/*****************************************************************
*
* read_code() - reads a specific-size code from the code file
*
****************************************************************/
unsigned int read_code(FILE *input, unsigned int code_size)
{
	unsigned int return_value;
	static int input_bit_count = 0;
	static unsigned long input_bit_buffer = 0L;

	/* The code file is treated as an input bit-stream. Each     */
	/*   character read is stored in input_bit_buffer, which     */
	/*   is 32-bit wide.                                         */

	/* input_bit_count stores the no. of bits left in the buffer */

	while (input_bit_count <= 24) {
		input_bit_buffer |= (unsigned long)getc(input) << (24 - input_bit_count);
		input_bit_count += 8;
	}

	return_value = input_bit_buffer >> (32 - code_size);
	input_bit_buffer <<= code_size;
	input_bit_count -= code_size;

	return(return_value);
}


/*****************************************************************
*
* write_code() - write a code (of specific length) to the file
*
****************************************************************/
void write_code(FILE *output, unsigned int code, unsigned int code_size)
{
	static int output_bit_count = 0;
	static unsigned long output_bit_buffer = 0L;

	/* Each output code is first stored in output_bit_buffer,    */
	/*   which is 32-bit wide. Content in output_bit_buffer is   */
	/*   written to the output file in bytes.                    */

	/* output_bit_count stores the no. of bits left              */

	output_bit_buffer |= (unsigned long)code << (32 - code_size - output_bit_count);
	output_bit_count += code_size;

	while (output_bit_count >= 8) {
		putc(output_bit_buffer >> 24, output);
		output_bit_buffer <<= 8;
		output_bit_count -= 8;
	}


	/* only < 8 bits left in the buffer                          */

}

/*****************************************************************
*
* compress() - compress the source file and output the coded text
*
****************************************************************/
void compress(FILE *input, FILE *output)
{
	int i;
	unsigned int nxtcode, character, strngcode, dex;
	
	nxtcode = 256;             
	for (i = 0; i<SIZEOFHASHTABLE; i++) codeval[i] = -1;
	i = 0;
	printf("Compressing File\n");
	strngcode = getc(input);    // Get the first code          
	while ((character = getc(input)) != (unsigned)EOF) //main loop for compression
	{
		dex = match(strngcode, character);
		if (codeval[dex] != -1)strngcode = codeval[dex];   
		else                                    //string not in table; Add string
		{
			if (nxtcode <= MAX_CODE)
			{
				prefix[dex] = strngcode; char_append[dex] = character;
				codeval[dex] = nxtcode++;
				
			}

			write_code(output, strngcode, CODE_SIZE);  // string is found  
			strngcode = character;            
		}                                   
	}                                   
	write_code(output, strngcode,CODE_SIZE); //Output the last code              
	write_code(output, MAXIMUMVALUE,CODE_SIZE);   
	// flush the output buffer ->
	write_code(output, 0, CODE_SIZE);   
	printf("Compression done!\n");
}


/*****************************************************************
*
* decompress() - decompress a compressed file to the orig. file
*
****************************************************************/
void decompress(FILE *input, FILE *output)
{
	int character;
	unsigned int nxtcode, ncode, ocode;
	unsigned char *strng;
	nxtcode = 256;       

	printf("Decompressing\n");
	ocode = read_code(input,CODE_SIZE);         
	putc(ocode, output);   
	character = ocode;
	while ((ncode = read_code(input,CODE_SIZE)) != (MAXIMUMVALUE))//decompression
	{
		
		if (ncode >= nxtcode) //Handling special cases like string+char+string
		{
			*decoded_string = character;
			strng = decode(decoded_string + 1, ocode);
		}
	
		else
		{
			strng = decode(decoded_string, ncode);
		}
		character = *strng;// outputing the decoded string in reverse order
		while (strng >= decoded_string) putc(*strng--, output);
		if (nxtcode <= MAX_CODE)//if possible, add new code to the string table
		{
			char_append[nxtcode] = character; prefix[nxtcode] = ocode;
			nxtcode++;
		}
		ocode = ncode;
	}
	printf("Decompression Done!\n");
}

//the hash routine tries to find a match for the prefix and character string in the table
int match(int prefixofhash, unsigned int hashchar)
{
	int offset,dex;
	dex = ((hashchar << HASHSHIFT)^prefixofhash);
	if (dex == 0) offset = 1;
	else offset = SIZEOFHASHTABLE - dex;
	while (TRUE)
	{

		if (prefix[dex] == prefixofhash && char_append[dex] == hashchar) return(dex); //if it is found, the index is returned
		if (codeval[dex] == -1) return(dex); //if is it not found, the first available index in the table is returned
		dex = dex -  offset;
		if (dex < 0) dex = dex + SIZEOFHASHTABLE;
	}
}

//This function decodes a string from the table storing it in a buffer
unsigned char *decode(unsigned char *BUF, unsigned int code)
{
	int i=0;
	while (code > 255)
	{
		*BUF = char_append[code];
		*BUF++;
		code = prefix[code];
		if (i++ >= MAX_CODE)
		{
			printf("ERROR DURING DECOMPRESSION\n");
			exit(-3);
		}
	}
	*BUF = code;

	return(BUF); //The buffer is returned in reverse order by the decoding program
}

