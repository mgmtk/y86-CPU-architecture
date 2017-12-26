/*
 * CS 261 PA1: Mini-ELF header verifier
 *
 * Name: Mitchell Mesecher
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "p1-check.h"


void usage_p1 ()
{
    printf("Usage: y86 <option(s)> mini-elf-file\n");
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
}

bool parse_command_line_p1 (int argc, char **argv, bool *header, char **file)
{
    //check for null params
    if(argc <= 1 ||argv == NULL|| header == NULL)
    {
        usage_p1();
        return false;
    }

    *header = false;
    char *optionStr = "hH";
    int opt;
    opterr = 0;
    bool printHelp = false;
    while((opt = getopt(argc, argv, optionStr))!= -1) 
    {
        switch(opt)
        {
            case 'h': printHelp = true; break;
            case 'H': *header = true; break;
            default: usage_p1(); return false;
        }
    }
    if(printHelp)
    {
    *header = false;
    usage_p1();
    return true;
    }
    else
    {

        *file = argv[optind];
        if(*file == NULL)
        {
            usage_p1();
            return false;
        }

        return true;
    }

}

bool read_header (FILE *file, elf_hdr_t *hdr)
{
    //read 16 bytes from file and check for magic number
    if(!file ||(fread(hdr, 16, 1, file) != 1) ||  hdr->magic != 4607045)
    {
        puts("Failed to read file");
        return false;
    }

    return true;

}

void dump_header (elf_hdr_t hdr)
{
    //print the first 16 bytes of the header
    unsigned char *byte_pointer = (unsigned char *)  &hdr;
    int i;
    printf("%s", "00000000 ");
    for(i = 0; i < 16; i++)
    {
        if(i == 8)
        {
            printf("%s", " ");
        }

        printf(" %02x", byte_pointer[i]);

    }
    //print the file descriptions
    printf("\n");
    printf("%s %d %s", "Mini-ELF version",hdr.e_version, "\n");
    printf("%s %#x %s", "Entry point",hdr.e_entry, "\n");
    printf("There are %d program headers, starting at offset %d (%#x)       \n",hdr.e_num_phdr,hdr.e_phdr_start, hdr.e_phdr_start);
    if(hdr.e_symtab == 0)
    {
        printf("%s", "There is no symbol table present \n");
    }
    else
    {
        printf("There is a symbol table starting at offset %d (%#x) \n",hdr.e_symtab, hdr.e_symtab);
    }
    if(hdr.e_strtab == 0)
    {
        printf("%s", "There is no string table present \n");

    }
    else
    {
        printf("There is a string table starting at offset %d (%#x) \n",hdr.e_strtab, hdr.e_strtab);
    }


}


