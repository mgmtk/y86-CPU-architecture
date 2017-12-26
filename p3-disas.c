/*
 * CS 261 PA3: Mini-ELF disassembler
 *
 * Name: Mitchell Mesecher
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p3-disas.h"

void spacePrinter(int printed);
void printReg(uint32_t reg);

void usage_p3 ()
{
    printf("Usage: y86 <option(s)> mini-elf-file\n");
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("  -a      Show all with brief memory\n");
    printf("  -f      Show all with full memory\n");
    printf("  -s      Show the program headers\n");
    printf("  -m      Show the memory contents (brief)\n");
    printf("  -M      Show the memory contents (full)\n");
    printf("  -d      Disassemble code contents\n");
    printf("  -D      Disassemble data contents\n");
}

bool parse_command_line_p3 (int argc, char **argv,
        bool *header, bool *segments, bool *membrief, bool *memfull,
        bool *disas_code, bool *disas_data, char **file)
{
    //check for passed in null values
    if(argc <= 1 ||argv == NULL || header == NULL || 
    membrief == NULL || segments == NULL || memfull == NULL || disas_code == NULL || disas_data == NULL)
    {
        usage_p3();
        return false;
    }

    *header = false;
    *segments = false;
    *membrief = false;
    *memfull = false;
    *disas_code = false;
    *disas_data = false;
    char *optionStr = "hHafsmMDd";
    int opt;
    opterr = 0;
    bool printHelp = false;
    
    //loop through commmand line arguements 
    while((opt = getopt(argc, argv, optionStr))!= -1) 
    {
        switch(opt)
        {
            case 'h': printHelp = true; break;
            case 'H': *header = true; break;
            case 'm': *membrief = true; break;
            case 'M': *memfull = true; break;
            case 's': *segments = true; break;
            case 'a': *header = true; *membrief = true; *segments = true; break; 
            case 'f': *header = true; *memfull = true; *segments = true; break;
            case 'd': *disas_code = true; break;
            case 'D': *disas_data = true; break;
            default: usage_p3(); return false;
        }
    }
    
    //checks if the help command was found and returns true
    if(printHelp)
    {
        *header = false;
        usage_p3();
        return true;
    }
    else if(*membrief && *memfull)
    {
        //checks if both the memory full and memory brief 
        //commands were found and returns false as this is invalid
        usage_p3();
        return false;
    }
    else
    {
        //sets the last command arg to the file char array 
        *file = argv[optind];
        if(*file == NULL || argc > optind + 1)
        {
            //prints usage and returns false if no file name was given
            usage_p3();
            return false;
        }
    
        return true;
    }
}

y86_inst_t fetch (y86_t cpu, memory_t memory) 
{

   y86_inst_t ins;
   
   //set instruction filed to 0
   memset(&ins, 0x00, sizeof(ins));
   
   //check for null or out of bounds paramaters
   if(memory == NULL || cpu.pc >= MEMSIZE || cpu.pc < 0)
   {
        ins.type = INVALID;
        return ins;
   } 
   
   //assigns the operation code
   uint64_t *p;
   uint8_t byte1 = memory[cpu.pc];
   uint8_t byte2;
   ins.opcode = byte1; 
   
   //switchs on the first byte to determine instruction 
   switch(byte1) 
   {
        case (0x00) : ins.type = HALT; ins.size = 1; 
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                
            }
            break;
            
        case (0x10) : ins.type = NOP; ins.size = 1; 
         if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                
            }
            break;
        
        //multiple cases for cmov
        case (0x20): case (0x21): case(0x22): case(0x23):
        case(0x24): case(0x25): case(0x26):
            ins.type = CMOV; ins.size = 2; ins.cmov = byte1 & 0x0F; 
       
            //tests for out of bounds   
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
       
            //sets and checks registers 
            byte2 = memory[cpu.pc + 1];    
            ins.ra = ((byte2 & 0xF0) >> 4); 
            ins.rb = (byte2 & 0x0F);     
            if(ins.ra > 0x07 || ins.rb > 0x07 || ins.cmov > 0x06) 
            {
                ins.type = INVALID; 
            }
            break;
                
        case (0x30) : ins.type = IRMOVQ; ins.size = 10; 
            //tests for out of bounds
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            
            //sets and checks registers
            byte2 = memory[cpu.pc + 1];
            ins.rb = (byte2 & 0x0F); 
            if(((byte2 & 0xF0) >> 4) != 0x0F || ins.rb > 0x07)
            {
                ins.type = INVALID; 
                break;
            }
            
            //assigns rest of instruction to val
            p = (uint64_t *) &memory[cpu.pc + 2]; 
            ins.value = *p;
            break;  
                
        case (0x40) : ins.type = RMMOVQ; ins.size = 10; 
            //tests out of bounds
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            
            //checks and sets registers
            byte2 = memory[cpu.pc + 1];
            ins.ra = ((byte2 & 0xF0) >> 4);
            ins.rb = (byte2 & 0x0F);
            if(ins.ra > 0x07 || ins.rb > 0x07)
            {
                ins.type = INVALID; 
                break;
            }
            
            //sets D with remaining instruction
            p = (uint64_t *) &memory[cpu.pc + 2];
            ins.d = *p;
            break;  
                
        case (0x50) : ins.type = MRMOVQ; ins.size = 10; 
            //tests for out of bounds
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            //checks ans assigns registers
            byte2 = memory[cpu.pc + 1];
            ins.ra = ((byte2 & 0xF0) >> 4);
            ins.rb = (byte2 & 0x0F);
            if(ins.ra > 0x07 || ins.rb > 0x07)
            {
                ins.type = INVALID; 
                break;
            }
            //assigns D value 
            p = (uint64_t *) &memory[cpu.pc + 2];
            ins.d = *p;
            break;  
        
        //multiple op cases        
        case (0x60): case(0x61): case(0x62): case(0x63):
            ins.type = OPQ; ins.size = 2; ins.op = byte1 & 0x0F;
            
            //checks for out of bounds
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            
            //assigns registers
            byte2 = memory[cpu.pc + 1];
            ins.ra = ((byte2 & 0xF0) >> 4);
            ins.rb = (byte2 & 0x0F);
            
            //invalid inst check
            if(ins.op > 0x03 || ins.ra > 0x07 || ins.rb > 0x07)
            {
                ins.type = INVALID;
            }
            break;
        
        //multiple jump cases
        case (0x70): case(0x71): case(0x72): case(0x73): 
        case(0x74): case(0x75): case(0x76):
            ins.type = JUMP; ins.size = 9; ins.jump = byte1 & 0x0F;
            //out of bounds test
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            //assign dest value
            p = (uint64_t *) &memory[cpu.pc + 1];
            ins.dest = *p;
            break;  
                
        case (0x80) : ins.type = CALL; ins.size = 9;
            //tests out of bounds
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            //assigns dest value
            p = (uint64_t *) &memory[cpu.pc + 1];
            ins.dest = *p;
            break;
                 
        case (0x90) : ins.type = RET; ins.size = 1; 
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                
            }
            break;
        
        case (0xA0) : ins.type = PUSHQ; ins.size = 2; 
            //tests out of bounds
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            //sets and checks registers
            byte2 = memory[cpu.pc + 1];
            ins.ra = ((byte2 & 0xF0) >> 4);
            if(((byte2 & 0x0F)) != 0x0F || ins.ra > 0x07)
            {
                ins.type = INVALID;          
            } 
            break;
                
        case (0xB0): ins.type = POPQ; ins.size = 2; 
            //checks out of bounds
            if(cpu.pc + ins.size >= MEMSIZE)
            {
                ins.type = INVALID;
                break;
            }
            //assigns registers
            byte2 = memory[cpu.pc + 1];
            ins.ra = ((byte2 & 0xF0) >> 4);
            if(((byte2 & 0x0F)) != 0x0F || ins.ra > 0x07)
            {
                ins.type = INVALID;        
            } 
            break;
        
        default: ins.type = INVALID;
   }
  
    
   return ins;
}



void printReg(uint32_t b) 
{
    //register printing helper method
    switch(b)
    {
        case 0x00: printf("%%rax"); break;
        case 0x01: printf("%%rcx"); break;
        case 0x02: printf("%%rdx"); break;
        case 0x03: printf("%%rbx"); break;
        case 0x04: printf("%%rsp"); break;
        case 0x05: printf("%%rbp"); break;
        case 0x06: printf("%%rsi"); break;
        case 0x07: printf("%%rdi"); break;
    }

}

void disassemble (y86_inst_t inst) 
{
   
    //switches on inst type
    switch(inst.type)
    {
        // correlating info for each case
        case HALT: printf("halt "); break;
        case NOP: printf("nop "); break;
        case CMOV: 
            //nested switch for cmov cases
            switch(inst.cmov)
            {
                case RRMOVQ: printf("rrmovq "); break; 
                case CMOVLE: printf("cmovle "); break;
                case CMOVL: printf("cmovl "); break;
                case CMOVE: printf("cmove "); break;
                case CMOVNE: printf("cmovne "); break;
                case CMOVGE: printf("cmovge " ); break;
                case CMOVG: printf("cmovg "); break;
                case BADCMOV: return; 
            } 
            printReg(inst.ra);
            printf(", ");
            printReg(inst.rb);
            break;
            
       case IRMOVQ: printf("irmovq "); 
            printf("%#lx, ", (uint64_t) inst.value); 
            printReg(inst.rb);
            break;
            
       case RMMOVQ: printf("rmmovq "); 
            printReg(inst.ra);
            printf(", %#lx(", (uint64_t) inst.d); 
            printReg(inst.rb);
            printf(")");
            break;
            
       case MRMOVQ: printf("mrmovq "); 
            printf("%#lx(", (uint64_t) inst.d); 
            printReg(inst.rb);
            printf("), ");
            printReg(inst.ra);
            break;
            
       case OPQ:
            //nested switch for op cases 
            switch(inst.op)
            {
                case ADD: printf("addq "); break;
                case SUB: printf("subq "); break;
                case AND: printf("andq "); break;
                case XOR: printf("xorq "); break;
                case BADOP:return;
               
            }
            printReg(inst.ra);
            printf(", ");
            printReg(inst.rb);
            break;
            
       case JUMP: 
            //nested switch for jump cases
            switch(inst.jump)
            {
                case JMP: printf("jmp "); break;
                case JLE: printf("jle "); break;
                case JL: printf("jl "); break;
                case JE: printf("je "); break;
                case JNE: printf("jne "); break;
                case JGE: printf("jge "); break;
                case JG: printf("jg "); break;
                case BADJUMP: return;

            }
            printf("%#lx", (uint64_t) inst.dest); 
            break;
            
       case CALL: printf("call "); printf("%#lx", (uint64_t) inst.dest); break;
       case RET: printf("ret "); break;
       case PUSHQ: printf("pushq "); printReg(inst.ra); break;
       case POPQ: printf("popq "); printReg(inst.ra); break;
       case INVALID: break;
       
     }


}

void disassemble_code (memory_t memory, elf_phdr_t *phdr, elf_hdr_t *hdr) 
{
    //tests for null parameters
    if(memory == NULL || phdr == NULL || hdr == NULL) 
    {
        return;
    }
    
    //assigns cpu program counter
    y86_t cpu;
    y86_inst_t ins;
    uint32_t addr = phdr->p_vaddr;
    cpu.pc = addr;
   
   
    printf("  0x%03lx:%31s%03lx code", (uint64_t) cpu.pc, "| .pos 0x", (uint64_t) cpu.pc);
    printf("\n");
      
    while(cpu.pc < addr + phdr->p_filesz) 
    {
        //checks if elh entry is the current instruction
        //prints start if so
        if(cpu.pc == hdr->e_entry)
        {
            printf("  0x%03lx:%31s", (uint64_t) cpu.pc, "| _start:");
            printf("\n");
        }
        ins = fetch(cpu, memory);
        
        //tests if ins type came back as invalid
        if(ins.type == INVALID)
        {
            //prints invalid op code
            printf("Invalid opcode: %#02x\n\n", ins.opcode);
            cpu.pc += ins.size;
            return;
        }
        
        //prints hex for current instruction
        printf("  0x%03lx: ", (uint64_t) cpu.pc);
        for(int i = cpu.pc; i < cpu.pc + ins.size; i++)
        {
            printf("%02x", memory[i]);
           
        }
        
        spacePrinter(ins.size);
        printf(" |   ");
        disassemble(ins);
        cpu.pc += ins.size;
        printf("\n");
    }
    printf("\n");

}

void disassemble_data (memory_t memory, elf_phdr_t *phdr)
{
    //tests for null parameters
    if(memory == NULL || phdr == NULL )
    {
        return;
    }

    //assigns cpu program counter
    y86_t cpu;
    uint32_t addr = phdr->p_vaddr;
    cpu.pc = addr;
    
    
    printf("  0x%03lx:", (uint64_t) cpu.pc);
    printf("%31s%03lx data\n","| .pos 0x", (uint64_t) cpu.pc);
    while(cpu.pc <  addr + phdr->p_filesz)
    {
        //prints hex 
        printf("  0x%03lx: ", (uint64_t) cpu.pc);
        for(int i = cpu.pc; i < cpu.pc + 8; i++)
        {
            printf("%02x", memory[i]);
        }
        printf("%6s","|"); 
        printf("   .quad ");
        
        //prints corrisponding data
        uint64_t *p;
        p = (uint64_t *) &memory[cpu.pc];
        printf("%p", (void *) *p);
        cpu.pc += 8;
        printf("\n"); 
        
        
    }
    printf("\n");
}

void spacePrinter(int printed)
{
    //prints needed spaces for formatting
    int spaces = 10 - printed;
    for(int i = 0; i < spaces; i++)
    {
        printf("  ");
    }
}

void disassemble_rodata (memory_t memory, elf_phdr_t *phdr)
{
    //checks for null paramters
    if(memory == NULL || phdr == NULL)
    {
        return;
    }
    y86_t cpu;
    uint32_t addr = phdr->p_vaddr;
    cpu.pc = addr;
    bool finished = false;
    int i;

    printf("  0x%03lx:  ", (uint64_t) cpu.pc);
    printf("%29s%03lx rodata\n","| .pos 0x", (uint64_t) cpu.pc);
    
    
    while(cpu.pc <  addr + phdr->p_filesz)
    {
        finished = false;
        printf("  0x%03lx: ", (uint64_t) cpu.pc);
        //executes until for first ten bytes or null is found  
        for(i = cpu.pc; i < cpu.pc + 10; i++)
        {
            printf("%02x", memory[i]);
            if(memory[i] == 0x00)
            {
                finished = true;
                spacePrinter(i - (cpu.pc -1));
                break;
            }
            
        }
        //print hex in characters
        i = cpu.pc;
        printf(" |   .string \"");
        while(memory[i] != 0x00)
        {
            printf("%c", memory[i++]);
        }
        
        //get number of bytes contained in this data
        int bytes = (i - cpu.pc) + 1;
        printf("\"");
        
        //if more hex needs to be printed for this string 
        if(!finished)
        {
            printf("\n");
            
            //offset by ten for the first ten bytes printed
            i = cpu.pc + 10;
            int j = 0;
            printf("  0x%03lx: ", (uint64_t) i);
            
            //loop until null character is found
            while(memory[i] != 0x00)
            {
                printf("%02x", memory[i++]);
                //if j is multiple of ten print | and newline
                if(++j % 10 == 0)
                {
                    printf(" |\n  0x%03lx: ", (uint64_t) i);
                }
            }
            printf("%02x", memory[i++]);
            spacePrinter((j  % 10) + 1);
            printf(" |");
        }
        cpu.pc += bytes; 
        printf("\n");
       
    }
    printf("\n");

}



