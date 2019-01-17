#include<commons.h>

struct Rom_header *header;

void print_meta(){
  header = (struct Rom_header*)rom_buffer;
  printf("string:%c\n",header->string[0]);
  printf("string:%c\n",header->string[1]);
  printf("string:%c\n",header->string[2]);
  printf("prg_count:%x\n",header->rom_prg_nos);
  printf("chr_count:%x\n",header->rom_chr_nos);
}

//Mapper 0
void mapper_init(){
  //mapping prg code
  unsigned int i,j;
  for(i=16,j=0x8000;j<=0xBFFF;i++,j++){
    main_mem[j] = rom_buffer[i];
  }

  if(header->rom_prg_nos == 1){
    for(i=16,j=0xC000;j<=0xFFFF;i++,j++){
      main_mem[j] = rom_buffer[i];
    }
    //mapping chr code
    for(i=0x0000,j=16+1*16384;i<=0x1FFF;i++,j++){
      ppu_mem[i] = rom_buffer[j];
    }
  }
  else if(header->rom_prg_nos == 2){
    for(i=16+1*16384,j=0xC000;j<=0xFFFF;i++,j++){
      main_mem[j] = rom_buffer[i];
    }
    //mapping chr code
    for(i=0x0000,j=16+2*16384;i<=0x1FFF;i++,j++){
      ppu_mem[i] = rom_buffer[j];
    }
  }
  else printf("Muahaha, mapper not implemented!!"); // :(
}
