#include<commons.h>

FILE *log_file;
FILE *rom;

void pause(){
  char num;
  scanf("%c",&num);
}
void memory_init(){
  log_file = fopen("log.txt","a");
  for(int i=0;i<0xFFFF;i++) {
    main_mem[i] = 0;
    ppu_mem[i] = 0;
  }
  rom = fopen("bal.nes","rb"); //Rom file
  if (rom == NULL){
    printf("Could not open ROM file");
    exit(0);
  }
  //filling Rom memory buffer ********************
  int c,offset=0;
  while(true) {
    c = fgetc(rom);
    if(feof(rom)) {
       break;
    }
    *(rom_buffer + offset++) = (int)c;
  }
}
