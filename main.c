/*
 * CS 261: Main driver
 *
 * Name: Mitchell Mesecher
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "p1-check.h"
#include "p2-load.h"
#include "p3-disas.h"
#include "p4-interp.h"

int main (int argc, char **argv)
{
    bool isheader = false;
    char *file = NULL;
    bool segments = false;
    bool membrief = false;
    bool memfull = false;
    bool disas_code = false;
    bool disas_data = false; 
    bool exec_normal = false;
    bool exec_debug = false;
    
    // parses the command line arguements 
    bool command = parse_command_line_p4(argc, argv, &isheader, &segments, &membrief, &memfull, &disas_code,
                                         &disas_data, &exec_normal, &exec_debug, &file);
    
    if(!command)
    {
        // returns exit failure if the command line is improper
        return EXIT_FAILURE;  
   
    }
    if(file == NULL)
    {
        // if file is null exit success (-h command found)
        return EXIT_SUCCESS;
        
    } 
    
    // opens the file
    FILE *input = fopen(file, "r");
    struct elf hdr;
    bool headerRead = read_header(input, &hdr);
    if(!headerRead)
    {
        //exit failure if program header can't be read
        return EXIT_FAILURE;
    }
  
    struct elf_phdr phdrs[hdr.e_num_phdr]; 
    int offset;
    //read program header information from the file into array of program headers
    for(int i = 0; i < hdr.e_num_phdr; i++) 
    {
        offset = hdr.e_phdr_start + (i * sizeof(elf_phdr_t));   
        if(!(read_phdr(input, offset, &phdrs[i])))
        {
            //exit failure if prgogram header can't be read from file
	        printf("Failed to read file");
            return EXIT_FAILURE;
        }
   
    }

    //allocate memory block of 4KBS
    memory_t memory = (memory_t) malloc(MEMSIZE);
    
    // load segments into virtual memory
    for(int k = 0; k < hdr.e_num_phdr; k++)
    {
        if(!(load_segment(input, memory, phdrs[k])))
        {
            // exit failure if a segment can't be loaded
	        printf("Failed to read file");
            free(memory);
            return EXIT_FAILURE;
        }
   
    }
    
    if(isheader)
    {
        // dump mini-elf header
        dump_header(hdr);
    }  
    if(segments)
    {
        // dump all program headers in file
        dump_phdrs(hdr.e_num_phdr, phdrs);
    }
    if(memfull)
    {
        //dump full memory
        dump_memory(memory, 0, MEMSIZE); 
    }
    else if(membrief)
    {
        // dump brief memory
        for(int j = 0; j < hdr.e_num_phdr; j++)
        {
            //virtual address of program header
            int vspace = phdrs[j].p_vaddr;
            dump_memory(memory, vspace, vspace + phdrs[j].p_filesz); 
        }
    
    }
    
    //converts each program header to assembly instructions
    int l;
    if(disas_code){
        printf("Disassembly of executable contents:\n");
        for(l = 0; l < hdr.e_num_phdr; l++)
        {
            //checks that each program header has the code flag
            if(phdrs[l].p_type == CODE)
            {
                disassemble_code(memory, &phdrs[l], &hdr);
            }
          
      
        }  
        
    }
    
    //writes the data of each program header

    if(disas_data)
    {
        printf("Disassembly of data contents:\n");
      for(l = 0; l < hdr.e_num_phdr; l++)
      {
           //writes the data in string read only form
           if(phdrs[l].p_type == DATA && phdrs[l].p_flag == 4)
           {
                disassemble_rodata(memory, &phdrs[l]);
           
           }
           ///writes the data of read and execute data
           else if(phdrs[l].p_type == DATA && phdrs[l].p_flag == 6)
           {
                 disassemble_data(memory, &phdrs[l]);
                
           }
      
      }  
    
    
    }

    //declare cpu and set fields to 0
    //initialize variables
    y86_t cpu;
    memset(&cpu, 0x00, sizeof(cpu));
    cpu.stat = AOK;
    int count = 0;
    y86_register_t valA = 0;
    y86_register_t valE = 0;
    bool cond = false;
    cpu.pc = hdr.e_entry; //entry of file
    
    //execute normal 
    if(exec_normal)
    {
        printf("Beginning execution at 0x%04x\n", hdr.e_entry);
        
        //loop executes while cpu status is ok 
        while(cpu.stat == AOK)
        {
            //fetch the instruction
            y86_inst_t ins = fetch(cpu, memory);
            cond = false;
            
            //debug and execute the instruction
            valE = decode_execute(&cpu, &cond, ins, &valA);
            
            //write value values to memory and regisiters
            //update program counter
            memory_wb_pc(&cpu, memory ,cond, ins, valE, valA);
            count++;
            
            //check to that pc didnt exceed memsize
            if(cpu.pc >= MEMSIZE)
            {
                cpu.stat = ADR;
                cpu.pc = 0xffffffffffffffff;
            }
        }
        //print cpu state
        cpu_state(cpu);
        printf("Total execution count: %d\n", count);

    }

    //execute debug mode
    if(exec_debug)
    {
        printf("Beginning execution at 0x%04x\n", hdr.e_entry);
        
        //execute while cpu status is ok
        while(cpu.stat == AOK)
        {
            //print cpu status on iteration
            cpu_state(cpu);
            
            //fetch intsruction
            y86_inst_t ins = fetch(cpu, memory);
            
            //print instruction
            printf("\nExecuting: ");  disassemble(ins); printf("\n");
            cond = false;
            
            //decode and execute
            valE = decode_execute(&cpu, &cond, ins, &valA);
            
            //write value to memory and registers
            //update program counter
            memory_wb_pc(&cpu, memory ,cond, ins, valE, valA);
            count++;
            
            //check if pc is exceeded memsize
            if(cpu.pc >= MEMSIZE)
            {
                cpu.stat = ADR;
                cpu.pc = 0xffffffffffffffff;
            }


        }
        //print cpu status
        cpu_state(cpu);
        printf("Total execution count: %d\n", count);

    }
    
    fclose(input);
    free(memory);
    return EXIT_SUCCESS;
}

