#include<commons.h>
extern void memory_init();
extern void print_meta();
extern void mapper_init();
extern void cpu_reset();
extern void ppu_reset();
extern uint cStep();
extern uint pStep();

struct CPU cpu;

u8 main_mem[0x10000];
u8 ppu_mem[0x10000]; //0x4000 limit
u8 rom_buffer[500000];

void sys_init(){
  memory_init();
  print_meta();
  mapper_init();
  ppu_reset();
  cpu_reset();
}

int main(){
  sys_init();
  uint cycles=0;
  while(cpu.terminate!=true){
    cycles = cStep() * 3;
    while(cycles-->0){
      if(cpu.opcode == 0xc094) pause();
      pStep();
    }
  }
  return 0;
}
