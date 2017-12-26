/*
 * CS 261 PA4: Mini-ELF interpreter
 *
 * Name: Mitchell Mesecher
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4-interp.h"
y86_register_t opIns(y86_register_t valB, y86_register_t *valA, y86_inst_t inst, y86_t *cpu);
y86_register_t regCheck(y86_rnum_t reg, y86_t *cpu);
bool getCond(y86_cmov_t mov, y86_t *cpu);
bool getJump(y86_jump_t jmp, y86_t *cpu);
void rightBack(y86_rnum_t reg, y86_t *cpu, y86_register_t  valE);
bool xOR(bool flag1, bool flag2);

void usage_p4 ()
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
    printf("  -e      Execute program\n");
    printf("  -E      Execute program (debug trace mode)\n");
}

bool parse_command_line_p4 (int argc, char **argv,
        bool *header, bool *segments, bool *membrief, bool *memfull,
        bool *disas_code, bool *disas_data,
        bool *exec_normal, bool *exec_debug, char **file)
    {
    //check for passed in null values
    if(argc <= 1 ||argv == NULL || header == NULL || 
    membrief == NULL || segments == NULL || memfull == NULL || disas_code == NULL 
    || disas_data == NULL || exec_debug == NULL || exec_normal == NULL)
    {
        usage_p4();
        return false;
    }

    *header = false;
    *segments = false;
    *membrief = false;
    *memfull = false;
    *disas_code = false;
    *disas_data = false;
    char *optionStr = "hHafsmMDdeE";
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
            case 'E': *exec_debug = true; break;
            case 'e': *exec_normal = true; break;
            default: usage_p4(); return false;
        }
    }
    
    //checks if the help command was found and returns true
    if(printHelp)
    {
        *header = false;
        usage_p4();
        return true;
    }
    else if(*membrief && *memfull)
    {
        //checks if both the memory full and memory brief 
        //commands were found and returns false as this is invalid
        usage_p4();
        return false;
    }
    else
    {
        //sets the last command arg to the file char array 
        *file = argv[optind];
        if(*file == NULL || argc > optind + 1)
        {
            //prints usage and returns false if no file name was given
            usage_p4();
            return false;
        }
    
        return true;
    }
}

void cpu_state (y86_t cpu)
{

    //print the cpu specifications
    printf("Y86 CPU state:\n");
    printf("  %%rip: %016lx   flags: Z%d S%d O%d     ",  cpu.pc, cpu.zf, cpu.sf, cpu.of);
    
    //switch for cpu status
    switch(cpu.stat)
    {
        case(1): printf("AOK\n"); break;
        case(2): printf("HLT\n"); break;
        case(3): printf("ADR\n"); break;
        case(4): printf("INS\n"); break;
    }
    printf("  %%rax: %016lx    %%rcx: %016lx \n",  cpu.rax,  cpu.rcx);
    printf("  %%rdx: %016lx    %%rbx: %016lx \n", cpu.rdx, cpu.rbx);
    printf("  %%rsp: %016lx    %%rbp: %016lx \n",   cpu.rsp,  cpu.rbp);
    printf("  %%rsi: %016lx    %%rdi: %016lx \n",  cpu.rsi, cpu.rdi);




}

y86_register_t decode_execute ( y86_t *cpu, bool *cond, y86_inst_t inst, y86_register_t *valA )
{
    //check for null values  
    if(cond == NULL || valA == NULL)
    {
        cpu->stat = INS;
        return 0;

    }

    //check for pc exceeding memsize
    if(cpu->pc >= MEMSIZE || cpu->pc < 0)
    {

        cpu->stat = ADR;
        cpu->pc = 0xffffffffffffffff;
        return 0;
    }

    y86_register_t valE = 0;
    y86_register_t valB;
    
    //switch on instructon type
    switch(inst.type)
    {
        case(HALT): cpu->stat = HLT; break; 
        
        case(NOP): break;
        
        case(CMOV): *valA = regCheck(inst.ra, cpu); 
            valE = *valA; 
            *cond = getCond(inst.cmov, cpu); 
            break; 
             
        case(IRMOVQ): valE = (uint64_t) inst.value; break;
            
        case(RMMOVQ): *valA = regCheck(inst.ra, cpu);
            valB =  regCheck(inst.rb, cpu);
            valE =  (uint64_t) inst.d + valB;
            break;
            
        case(MRMOVQ): valB = regCheck(inst.rb, cpu); 
            valE = (uint64_t) inst.d + valB;
            break;
            
        case(OPQ): *valA = regCheck(inst.ra, cpu); 
            valB = regCheck(inst.rb, cpu); 
            valE = opIns(valB, valA, inst, cpu);
            break;
            
        case(JUMP): *cond = getJump(inst.jump, cpu); break;
        
        case(CALL): valB = cpu->rsp; valE =  valB - 8; break;
        
        case(RET): *valA = cpu->rsp;
            valB = cpu->rsp; 
            valE = valB + 8;
            break;
            
        case(PUSHQ): *valA = regCheck(inst.ra, cpu); 
            valB = cpu->rsp;
            valE = valB - 8; 
            break;
            
        case(POPQ): *valA = cpu->rsp; 
            valB = cpu->rsp; 
            valE = valB + 8; 
            break;
            
        case(INVALID): cpu->stat = INS; break;
        default: cpu->stat = INS; break;

    }

    return valE;
}

void memory_wb_pc (y86_t *cpu, memory_t memory, bool cond, y86_inst_t inst,
        y86_register_t valE, y86_register_t valA)
{
    y86_register_t valM;
    uint64_t *p;
    
    //check for pc exceeding memsize
    if(cpu->pc >= MEMSIZE || cpu->pc < 0 || memory == NULL || valE < 0 || valA < 0)
    {
        cpu->stat = ADR;
        cpu->pc = 0xffffffffffffffff;
        return;
    }

    //switch on instruction type
    switch(inst.type)
    {
        case(HALT): cpu->pc = 0; cpu->zf = false; cpu->sf = false; cpu->of = false; break;
        
        case(NOP):  cpu->pc += inst.size; break;
        
        case(CMOV):
            if(cond)
            {
                rightBack(inst.rb, cpu, valE);
            }
            cpu->pc += inst.size;
            break;
            
        case(IRMOVQ): rightBack(inst.rb, cpu, valE); 
            cpu->pc += inst.size; 
            break;
            
        case(RMMOVQ): 
            if(valE >=MEMSIZE) 
            {
                cpu->stat = ADR; 
                cpu->pc = 0xffffffffffffffff;
                break;
            }
            p = (uint64_t *) &memory[valE];
            *p = valA; cpu->pc += inst.size;
            printf("Memory write to 0x%04lx: 0x%lx \n", valE, valA);
            break;
            
        case(MRMOVQ): 
            if(valE >=MEMSIZE)
            {
                cpu->stat = ADR; 
                cpu->pc = 0xffffffffffffffff;
                break; 
            }
            p = (uint64_t *) &memory[valE];
            valM = *p;
            rightBack(inst.ra, cpu, valM); 
            cpu->pc += inst.size;
            break;
            
        case(OPQ): 
            rightBack(inst.rb, cpu, valE); 
            cpu->pc += inst.size; 
            break;
            
        case(JUMP):
            if(cond)
            {
                cpu->pc = inst.dest;
                break;
            }
            cpu->pc += inst.size;
            break;
            
        case(CALL):  
            if(valE >=MEMSIZE)
            { 
                cpu->stat = ADR;
                cpu->pc = 0xffffffffffffffff;
                break;  
            }
            p = (uint64_t *) &memory[valE];
            *p = cpu->pc += inst.size; cpu->rsp = valE; 
            cpu->pc = inst.dest;
            printf("Memory write to 0x%04lx: 0x%lx \n", valE, *p);
            break;
            
        case(RET): 
            if(valA >=MEMSIZE)
            { 
                cpu->stat = ADR; 
                cpu->pc = 0xffffffffffffffff;
                break; 
            }
            p = (uint64_t *) &memory[valA];
            valM = *p;
            cpu->rsp = valE;
            cpu->pc = valM; 
            break;
            
        case(PUSHQ): 
            if(valE >=MEMSIZE)
            { 
                cpu->stat = ADR; 
                cpu->pc = 0xffffffffffffffff;
                break; 
            }
            p = (uint64_t *) &memory[valE];
            *p = valA; cpu->rsp = valE; 
            cpu->pc += inst.size;
            printf("Memory write to 0x%04lx: 0x%lx \n",  valE, valA);
            break;
            
        case(POPQ):
            if(valA >=MEMSIZE)
            { 
                cpu->stat = ADR; 
                cpu->pc = 0xffffffffffffffff;
                break; 
            }
            p = (uint64_t *) &memory[valA];
            valM = *p;
            cpu->rsp = valE; 
            rightBack(inst.ra, cpu, valM); 
            cpu->pc += inst.size; 
            break;
            
        case(INVALID): cpu->stat = INS; break;
    }


}

y86_register_t opIns(y86_register_t valB, y86_register_t *valA, y86_inst_t inst, y86_t *cpu)
{

    int64_t sValE = 0;
    y86_register_t ValE = 0;
    int64_t sValB = valB;
    int64_t sValA = *valA;
    
    //check flags for opq instruction
    switch(inst.op)
    {
        case(ADD):  sValE = sValB + sValA;
        
        //set overflow of addition
        cpu->of = (sValB < 0 && sValA < 0 && sValE > 0) || (sValB > 0 && sValA > 0 && sValE < 0);
            ValE = sValE;
            break;
        
        //set overflow of subtraction
        case(SUB): sValE = sValB - sValA;
            cpu->of = ((sValB < 0 && sValA > 0 && sValE > 0) || (sValB > 0 && sValA < 0 && sValE < 0 )); 
            ValE = sValE;
            break;

        case(AND): ValE = valB & *valA; break;

        case(XOR): ValE = *valA ^ valB; break;

        case(BADOP): cpu->stat = INS; return ValE;
        default : cpu ->stat = INS; return ValE;

    }
    
    //set sign or zero flag for all cases
    cpu->sf = (ValE >> 63 == 1);
    cpu->zf = (ValE == 0);

return ValE;

}

void rightBack(y86_rnum_t reg, y86_t *cpu, y86_register_t  valX)
{
    //helper method to write back value to register
    switch(reg)
    {
        case(0): cpu->rax = valX; break;
        case(1): cpu->rcx = valX; break;
        case(2): cpu->rdx = valX; break;
        case(3): cpu->rbx = valX; break;
        case(4): cpu->rsp = valX; break;
        case(5): cpu->rbp = valX; break;
        case(6): cpu->rsi = valX; break;
        case(7): cpu->rdi = valX; break;
        case(BADREG): cpu->stat = INS;  
        default: break;
    }

}

y86_register_t regCheck(y86_rnum_t reg, y86_t *cpu)
{
    //helper mehtod switch statment to retrieve register
    switch(reg)
    {
        case(0): return cpu->rax;
        case(1): return cpu->rcx;
        case(2): return cpu->rdx;
        case(3): return cpu->rbx;
        case(4): return cpu->rsp;
        case(5): return cpu->rbp;
        case(6): return cpu->rsi;
        case(7): return cpu->rdi;
        case(BADREG): cpu->stat = INS; return 0;
        default: return 0;
    }

}

bool xOR(bool flag1, bool flag2)
{
    //check XOR condition
    return (flag1 && !flag2) || (!flag1 && flag2);
}

bool getCond(y86_cmov_t mov, y86_t *cpu)
{
    //switch statement helper method to check cmov condition
    bool cond = false;
    switch(mov)
    {
        case(RRMOVQ):
            cond = true;
            break;
        case(CMOVLE):
            if(cpu->zf || xOR(cpu->sf ,cpu->of))
            {
                cond = true;
            }
            break;
        case(CMOVL):
            if(xOR(cpu->sf , cpu->of))
            {
                cond =  true;
            }
            break;
        case(CMOVE):
            if(cpu->zf)
            {
                cond =  true;
            }
            break;
        case(CMOVNE):
            if(!cpu->zf)
            {
                cond =  true;
            }
            break;
        case(CMOVGE):
            if(cpu->sf == cpu->of)
            {
                cond =  true;
            }
            break;
        case(CMOVG):
            if(!cpu->zf && cpu->sf == cpu->of)
            {
                cond =  true;
            }
            break;
        case(BADCMOV): cpu->stat = INS;
    }

    return cond;
}

bool getJump(y86_jump_t jmp, y86_t *cpu)
{
    //switch statement helper method to check jump condition
    bool cond = false;
    switch(jmp)
    {
        case(JMP): 
            cond = true;
            break;
        case(JLE):
            if(cpu->zf || xOR(cpu->sf, cpu->of))
            {
                cond = true;
            }
            break;
        case(JL):
            if(xOR(cpu->sf, cpu->of))
            {
                cond =  true;
            }
            break;
        case(JE):
            if(cpu->zf)
            {
                cond =  true;
            }
            break;
        case(JNE):
            if(!cpu->zf)
            {
                cond =  true;
            }
            break;
        case(JGE):
            if(cpu->sf == cpu->of)
            {
                cond =  true;
            }
            break;
        case(JG):
            if(!cpu->zf && cpu->sf == cpu->of) 
            {
                cond =  true;
            }
            break;
        case(BADJUMP): cpu->stat = INS;
    }

    return cond;
}
