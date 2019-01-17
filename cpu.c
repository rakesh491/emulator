#include<commons.h>
extern u8 readRegs();
extern void writeRegs();

FILE *log_file;
struct CPU cpu;
struct PPU ppu;

struct stepInfo{
  u16 address;
  u16 pc;
  u8 mode;
};
enum interrupt{
  int_none,
  int_NMI,
  int_IRQ
};
enum addr_mode { //types of addressing modes used by the cpu
  invalid_mode,
  absolute,
  absolute_X,
  absolute_Y,
  accumulator,
  immediate,
  implied,
  indexed_indirect,
  indirect,
  indirect_indexed,
  relative,
  zero_page,
  zero_page_X,
  zero_page_Y
};
unsigned int inst_modes[256] = { //Addressing modes for the instructions
  /*     0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F */
  /*0*/  6,7,6,7,11,11,11,11,6,5,4,5,1,1,1,1,
  /*1*/	 10,9,6,9,12,12,12,12,6,3,6,3,2,2,2,2,
  /*2*/	 1,7,6,7,11,11,11,11,6,5,4,5,1,1,1,1,
  /*3*/	 10,9,6,9,12,12,12,12,6,3,6,3,2,2,2,2,
  /*4*/	 6,7,6,7,11,11,11,11,6,5,4,5,1,1,1,1,
  /*5*/	 10,9,6,9,12,12,12,12,6,3,6,3,2,2,2,2,
  /*6*/	 6,7,6,7,11,11,11,11,6,5,4,5,8,1,1,1,
  /*7*/	 10,9,6,9,12,12,12,12,6,3,6,3,2,2,2,2,
  /*8*/	 5,7,5,7,11,11,11,11,6,5,6,5,1,1,1,1,
  /*9*/	 10,9,6,9,12,12,13,13,6,3,6,3,2,2,3,3,
  /*A*/	 5,7,5,7,11,11,11,11,6,5,6,5,1,1,1,1,
  /*B*/	 10,9,6,9,12,12,13,13,6,3,6,3,2,2,3,3,
  /*C*/	 5,7,5,7,11,11,11,11,6,5,6,5,1,1,1,1,
  /*D*/	 10,9,6,9,12,12,12,12,6,3,6,3,2,2,2,2,
  /*E*/	 5,7,5,7,11,11,11,11,6,5,6,5,1,1,1,1,
  /*F*/  10,9,6,9,12,12,12,12,6,3,6,3,2,2,2,2
};
unsigned int inst_size[256] = { //Instruction sizes
  /*     0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F */
  /*0*/  1,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*1*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
  /*2*/  3,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*3*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
  /*4*/  1,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*5*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
  /*6*/  1,2,1,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*7*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
  /*8*/  2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*9*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
  /*A*/  2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*B*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
  /*C*/  2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*D*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
  /*E*/  2,2,2,2,2,2,2,2,1,2,1,2,3,3,3,3,
  /*F*/  2,2,1,2,2,2,2,2,1,3,1,3,3,3,3,3,
};
unsigned int inst_cycles[256] = { //Instructions cycles without conditional cycles
  /*     0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F */
  /*0*/  7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
  /*1*/  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  /*2*/  6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
  /*3*/  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  /*4*/  6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
  /*5*/  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  /*6*/  6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
  /*7*/  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  /*8*/  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  /*9*/  2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
  /*A*/  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  /*B*/  2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
  /*C*/  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  /*D*/  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  /*E*/  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  /*F*/  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};
unsigned int inst_Page_cycles[256] = { //Instructions cycles with page boundary crossed
  /*     0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F */
  /*0*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*1*/  1,1,0,0,0,0,0,0,0,1,0,0,1,1,0,0,
  /*2*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*3*/  1,1,0,0,0,0,0,0,0,1,0,0,1,1,0,0,
  /*4*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*5*/  1,1,0,0,0,0,0,0,0,1,0,0,1,1,0,0,
  /*6*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*7*/  1,1,0,0,0,0,0,0,0,1,0,0,1,1,0,0,
  /*8*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*9*/  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*A*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*B*/  1,1,0,1,0,0,0,0,0,1,0,1,1,1,1,1,
  /*C*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*D*/  1,1,0,0,0,0,0,0,0,1,0,0,1,1,0,0,
  /*E*/  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /*F*/  1,1,0,0,0,0,0,0,0,1,0,0,1,1,0,0,
};

u8 GetFlags(){
  u8 flags = 0x0000;
	flags |= (cpu.C << 0);
	flags |= (cpu.Z << 1);
	flags |= (cpu.I << 2);
	flags |= (cpu.D << 3);
	flags |= (cpu.B << 4);
	flags |= (cpu.U << 5);
	flags |= (cpu.V << 6);
	flags |= (cpu.N << 7);
	return flags;
}
void SetFlags(u8 flags){
  cpu.C = (flags >> 0) & 1;
  cpu.Z = (flags >> 1) & 1;
	cpu.I = (flags >> 2) & 1;
	cpu.D = (flags >> 3) & 1;
	cpu.B = (flags >> 4) & 1;
	cpu.U = (flags >> 5) & 1;
	cpu.V = (flags >> 6) & 1;
  cpu.N = (flags >> 7) & 1;
}
u8 cread(u16 address){
  switch(address){
    case 0x0000 ... 0x1FFF: return main_mem[address%0x0800];
    case 0x2000 ... 0x3FFF: return readRegs(0x2000 + address%8);
    case 0x4000 ... 0x4013: return main_mem[address];
    case 0x4014: printf("DMA read\n");pause();return readRegs(address);
    case 0x4015 ... 0x5FFF: return main_mem[address];
    case 0x8000 ... 0xBFFF: return main_mem[address];
    case 0xC000 ... 0xFFFF: return main_mem[address];
    default:printf("ERROR invalid cpu read: 0x%x\n",address);exit(0);
  }
  return 0;
}
void cwrite(u16 address, u8 value){
  switch(address){
    case 0x0000 ... 0x1FFF: main_mem[address%0x0800] = value;break;
    case 0x2000 ... 0x3FFF: writeRegs(0x2000+(address%8), value);break;
    case 0x4000 ... 0x4013: main_mem[address] = value;break;
    case 0x4014: printf("DMA write\n");pause();writeRegs(address,value);break;
    case 0x4015 ... 0x5FFF: main_mem[address] = value;break;
    case 0x6000 ... 0xFFFF: main_mem[address] = value;break;
    default:printf("ERROR invalid cpu write\n");exit(0);
  }
}
u16 cread16(u16 address){
  u8 low = (u8)cread(address);
  u8 high = (u8)cread(address+1);
  return (u16)((high<<8)|low);
}
u16 cread16bug(u16 address){
  u16 a = address;
  u16 b = ((u16)(address&0x00FF) == 0x00FF)?((u16)(address&0xFF00)):((u16)(address+1));
  u8 low = (u8)cread(a);
  u8 high = (u8)cread(b);
  return (u16)((high<<8)|low);
}
void push(u8 value){
  cwrite((u16)(0x100|cpu.sp),value);
  cpu.sp--;
}
u8 pull(){
  cpu.sp++;
  return cread(0x100|cpu.sp);
}
void push16(u16 value){
  u8 high = value>>8;
  u8 low = value&0xFF;
  push(high);
  push(low);
}
u16 pull16(){
  u8 low = pull();
  u8 high = pull();
  return (u16)((high<<8)|low);
}
uint diff_Page(u16 addr1,u16 addr2){
  if((addr1&0xFF00)!=(addr2&0xFF00))
  return true;
  else
  return false;
}
void add_branch_cycles(struct stepInfo info){
  cpu.cycles++;
  if(diff_Page(info.pc,info.address) == 1){
    cpu.cycles++;
  }
}
void SetZ(u8 value){
  cpu.Z = (value==0)?1:0;
}
void SetN(u8 value){
  cpu.N = (value&0x80)?1:0;
}
void SetZN(u8 value){
  cpu.Z = (value==0)?1:0;
  cpu.N = (value&0x80)?1:0;
}
void triggerNMI(){
  cpu.interrupt = int_NMI;
}
void triggerIRQ(){
  if(cpu.I == 0)
  cpu.interrupt = int_IRQ;
}
void compare(u8 a,u8 b){
  SetZN((u8)a-(u8)b);
  cpu.C = (a>=b)?1:0;
}
void mode_addressing(uint mode,u16 *address,uint *pageCrossed){
  switch(mode){
    case absolute:
      *address = (uint16_t)cread16(cpu.pc + 1);
      break;
    case absolute_X:{
      *address = cread16(cpu.pc + 1) + (uint16_t)cpu.x;
      *pageCrossed = diff_Page(*address - cpu.x, *address);
      break;
    }
    case absolute_Y:{
      *address = cread16(cpu.pc + 1) + (uint16_t)cpu.y;
      *pageCrossed = diff_Page(*address - cpu.y, *address);
      break;
    }
    case accumulator: *address = 0;break;
    case immediate: *address = (cpu.pc + 1);break;
    case implied: *address = (cpu.pc + 0);break;
    case indexed_indirect:{
      *address = (uint16_t)(cread(cpu.pc+1)+cpu.x) & 0x00FF;
      *address = (uint16_t)cread16bug(*address);
      break;
    }
    case indirect: *address = cread16bug(cread16(cpu.pc+1));break;
    case indirect_indexed:{
      *address = cread16bug(cread(cpu.pc+1)) + cpu.y;
      *pageCrossed = diff_Page(*address - cpu.y, *address);
      break;
    }
    case relative:{ // PC moves to next inst and relative offset is applied to the new PC address
      uint8_t offset = cread(cpu.pc + 1);
      if(offset < 0x80) *address = cpu.pc + 2 + offset;
      else *address = cpu.pc + 2 + offset - 0x100;
      break;
    }
    case zero_page: *address = (uint8_t)(cread(cpu.pc + 1))&0x00FF;break;
    case zero_page_X: *address = (uint8_t)(cread(cpu.pc + 1) + cpu.x)&0x00FF;break;
    case zero_page_Y: *address = (uint8_t)(cread(cpu.pc + 1) + cpu.y)&0x00FF;break;
  }
  cpu.fetch_address = *address;
}
void execute(u8 opcode,struct stepInfo info){
  u8 a,b,c,value;
  switch(opcode){
    case 0x61:
    case 0x65:
    case 0x69:
    case 0x6D:
    case 0x71:
    case 0x75:
    case 0x79:
    case 0x7D:{//ADC
      a = cpu.a;
      b = cread(info.address);
      c = cpu.C;
      cpu.a = a+b+c;
      SetZN(cpu.a);
      cpu.C = (a+b+c > 0xFF)?1:0;
      cpu.V = (((a^b)&0x80)==0 && ((a^cpu.a)&0x80)!=0)?1:0;
      break;
    }

    case 0x21:
    case 0x25:
    case 0x29:
    case 0x2D:
    case 0x31:
    case 0x35:
    case 0x39:
    case 0x3D:{//2 AND
      cpu.a = cpu.a & cread(info.address);
      SetZN(cpu.a);
      break;
    }

    case 0x06:
    case 0x0A:
    case 0x0E:
    case 0x16:
    case 0x1E:{//3 ASL
      if(info.mode == accumulator){
        cpu.C = (cpu.a >> 7) & 1;
        cpu.a <<= 1;
        SetZN(cpu.a);
      }
      else{
        value = cread(info.address);
        cpu.C = (value >> 7) & 1;
        value <<= 1;
        cwrite(info.address, value);
        SetZN(value);
      }
      break;
    }

    case 0x90:{//4 BCC
      if(cpu.C == 0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0xB0:{//5 BCS
      if(cpu.C != 0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0xF0:{//6 BEQ
      if(cpu.Z !=0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0x24:
    case 0x2C:{//7 BIT
      value = cread(info.address);
      cpu.V = (value >> 6) & 1;
      SetZ(value & cpu.a);
      SetN(value);
      break;
    }

    case 0x30:{//8 BMI
      if(cpu.N != 0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0xD0:{//9 BNE
      if(cpu.Z == 0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0x10:{//10 BPL
      if(cpu.N == 0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0x00:{//11 BRK
      struct stepInfo info = {0,0};
      push16(cpu.pc);
      execute(0x08,info);
      execute(0x78,info);
      cpu.pc = cread16(0xFFFE);
      break;
    }

    case 0x50:{//12 BVC
      if(cpu.V == 0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0x70:{//12 BVS
      if(cpu.V != 0){
        cpu.pc = info.address;
        add_branch_cycles(info);
      }
      break;
    }

    case 0x18:{//13 CLC
      cpu.C = 0;
      break;
    }

    case 0xD8:{//14 CLD
      cpu.D = 0;
      break;
    }

    case 0x58:{//15 CLI
      cpu.I = 0;
      break;
    }

    case 0xB8:{//16 CLV
      cpu.V = 0;
      break;
    }

    case 0xC1:
    case 0xC5:
    case 0xC9:
    case 0xCD:
    case 0xD1:
    case 0xD5:
    case 0xD9:
    case 0xDD:{//17 CMP
      value = (u8)cread(info.address);
      compare(cpu.a, value);
      break;
    }

    case 0xE0:
    case 0xE4:
    case 0xEC:{//18 CPX
      value = cread(info.address);
      compare(cpu.x,value);
      break;
    }

    case 0xC0:
    case 0xC4:
    case 0xCC:{//19 CPY
      value = cread(info.address);
      compare(cpu.y,value);
      break;
    }

    case 0xC6:
    case 0xCE:
    case 0xD6:
    case 0xDE:{//20 DEC
      value = cread(info.address) - 1;
      cwrite(info.address,value);
      SetZN(value);
      break;
    }

    case 0xCA:{//21 DEX
      cpu.x--;
      SetZN(cpu.x);
      break;
    }

    case 0x88:{//22 DEY
      cpu.y--;
      SetZN(cpu.y);
      break;
    }

    case 0x41:
    case 0x51:
    case 0x45:
    case 0x55:
    case 0x49:
    case 0x59:
    case 0x4D:
    case 0x5D:{//23 EOR
      cpu.a = cpu.a^cread(info.address);
      SetZN(cpu.a);
      break;
    }

    case 0xE6:
    case 0xEE:
    case 0xF6:
    case 0xFE:{//24 INC
      value = cread(info.address) + 1;
      cwrite(info.address,value);
      SetZN(value);
      break;
    }

    case 0xE8:{//25 INX
      cpu.x++;
      SetZN(cpu.x);
      break;
    }

    case 0xC8:{//26 INY
      cpu.y++;
      SetZN(cpu.y);
      break;
    }

    case 0x4C:
    case 0x6C:{//27 JMP
      cpu.pc = info.address;
      break;
    }

    case 0x20:{//28 JSR
      push16(cpu.pc - 1);
      cpu.pc = info.address;
      break;
    }

    case 0xA1:
    case 0xA5:
    case 0xA9:
    case 0xB1:
    case 0xB5:
    case 0xB9:
    case 0xBD:
    case 0xAD:{//29 LDA
      cpu.a = (u8)cread(info.address);
      SetZN(cpu.a);
      break;
    }

    case 0xA2:
    case 0xA6:
    case 0xAE:
    case 0xB6:
    case 0xBE:{//30 LDX
      cpu.x = cread(info.address);
      SetZN(cpu.x);
      break;
    }

    case 0xA0:
    case 0xA4:
    case 0xAC:
    case 0xB4:
    case 0xBC:{//31 LDY
      cpu.y = cread(info.address);
      SetZN(cpu.y);
      break;
    }

    case 0x46:
    case 0x4A:
    case 0x4E:
    case 0x56:
    case 0x5E:{//32 LSR
      if(info.mode == accumulator){
        cpu.C = cpu.a & 1;
        cpu.a >>=(u8)1;
        SetZN(cpu.a);
      }
      else{
        value = cread(info.address);
        cpu.C = value & 1;
        value >>=(u8)1;
        cwrite(info.address,value);
        SetZN(value);
      }
      break;
    }

    case 0x80:
    case 0x82:
    case 0xC2:
    case 0xE2:
    case 0x04:
    case 0x14:
    case 0x34:
    case 0x44:
    case 0x54:
    case 0x64:
    case 0x74:
    case 0xD4:
    case 0xF4:
    case 0x89:
    case 0x1A:
    case 0x3A:
    case 0x5A:
    case 0x7A:
    case 0xDA:
    case 0xEA:
    case 0xFA:
    case 0x0C:
    case 0x1C:
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
    //33 NOP
    break;

    case 0x01:
    case 0x05:
    case 0x09:
    case 0x0D:
    case 0x11:
    case 0x15:
    case 0x19:
    case 0x1D:{//34 ORA
      cpu.a = cpu.a | cread(info.address);
      SetZN(cpu.a);
      break;
    }

    case 0x48:{//35 PHA
      push(cpu.a);
      break;
    }

    case 0x08:{//36 PHP
      push(GetFlags() | 0x10); //TODO review this and plp
      break;
    }

    case 0x68:{//37 PLA
      cpu.a = pull();
      SetZN(cpu.a);
      break;
    }

    case 0x28:{//38 PLP
      SetFlags((pull()&0xEF) | 0x20); //TODO modified
      break;
    }

    case 0x26:
    case 0x2A:
    case 0x2E:
    case 0x36:
    case 0x3E:{//39 ROL
      if(info.mode == accumulator){
        c = cpu.C;
        cpu.C = (u8)((cpu.a >> 7) & 1);
        cpu.a = (u8)((cpu.a << 1) | c);
        SetZN(cpu.a);
      }
      else{
        c = cpu.C;
        value = cread(info.address);
        cpu.C = (u8)((value >> 7) & 1);
        value = (u8)((value << 1) | c);
        cwrite(info.address,value);
        SetZN(value);
      }
      break;
    }

    case 0x66:
    case 0x6A:
    case 0x6E:
    case 0x76:
    case 0x7E:{//40 ROR
      if(info.mode == accumulator){
        c = cpu.C;
        cpu.C = cpu.a & 0x0001;
        cpu.a = (u8)((u8)(cpu.a >> 1) | (u8)(c << 7));
        SetZN(cpu.a);
      }
      else{
        c = cpu.C;
        value = cread(info.address);
        cpu.C = value & 1;
        value = (u8)((u8)(value >> 1) | (u8)(c << 7));
        cwrite(info.address,value);
        SetZN(value);
      }
      break;
    }

    case 0x40:{//41 RTI
      SetFlags((pull() & 0xEF) | 0x20);
      cpu.pc = pull16();
      break;
    }

    case 0x60:{//42 RTS
      cpu.pc = (u16)(pull16() + 1);
      break;
    }

    case 0xE1:
    case 0xF1:
    case 0xE5:
    case 0xF5:
    case 0xE9:
    case 0xF9:
    case 0xEB:
    case 0xED:
    case 0xFD:{//43 SBC
      a = cpu.a;
      b = cread(info.address);
      c = cpu.C;
      cpu.a = a - b - (1 - c);
      SetZN(cpu.a);
      cpu.C = (a-b-(1-c) >= 0)?1:0;
      cpu.V = (((a^b) & 0x80) != 0 && ((a^cpu.a) & 0x80) != 0)?1:0; //TODO might be wrong
      break;
    }

    case 0x38:{//44 SEC
      cpu.C = 1;
      break;
    }

    case 0xF8:{//45 SED
      cpu.D = 1;
      break;
    }

    case 0x78:{//46 SEI
      cpu.I = 1;
      break;
    }

    case 0x81:
    case 0x91:
    case 0x85:
    case 0x95:
    case 0x99:
    case 0x8D:
    case 0x9D:{//47 STA
      cwrite(info.address,cpu.a);
      break;
    }

    case 0x86:
    case 0x96:
    case 0x8E:{//48 STX
      cwrite(info.address,cpu.x);
      break;
    }

    case 0x84:
    case 0x94:
    case 0x8C:{//49 STY
      cwrite(info.address,cpu.y);
      break;
    }

    case 0xAA:{//50 TAX
      cpu.x = cpu.a;
      SetZN(cpu.x);
      break;
    }

    case 0xA8:{//51 TAY
      cpu.y = cpu.a;
      SetZN(cpu.y);
      break;
    }

    case 0xBA:{//52 TSX
      cpu.x = cpu.sp;
      SetZN(cpu.x);
      break;
    }

    case 0x8A:{//53 TXA
      cpu.a = cpu.x;
      SetZN(cpu.a);
      break;
    }

    case 0x9A:{//54 TXS
      cpu.sp = cpu.x;
      break;
    }

    case 0x98:{//55 TYA
      cpu.a = cpu.y;
      SetZN(cpu.a);
      break;
    }

    //UNOFFICIAL OPCODES
    case 0xA7:
    case 0xB7:
    case 0xAF:
    case 0xBF:
    case 0xB3:
    case 0xA3:{//LAX
      cpu.a = cread(info.address);
      cpu.x = cread(info.address);
      SetZ(cpu.a & cpu.x);
      SetN(cpu.a | cpu.x);
      break;
    }

    case 0x87:
    case 0x97:
    case 0x83:
    case 0x8F:{//SAX
      value = (u8)((u8)cpu.a & (u8)cpu.x);
      cwrite(info.address,value);
      break;
    }

    case 0xC7:
    case 0xD7:
    case 0xCF:
    case 0xDF:
    case 0xDB:
    case 0xC3:
    case 0xD3:{//DCP
      value = cread(info.address) - 1; //DEC
      cwrite(info.address,value);
      SetZN(value);

      value = (u8)cread(info.address); //CMP
      compare(cpu.a, value);
      break;
    }

    case 0xE7:
    case 0xF7:
    case 0xEF:
    case 0xFF:
    case 0xFB:
    case 0xE3:
    case 0xF3:{//ISB/ISC
      value = cread(info.address) + 1; //INC
      cwrite(info.address,value);
      SetZN(value);

      a = cpu.a; //SBC
      b = cread(info.address);
      c = cpu.C;
      cpu.a = a - b - (1 - c);
      //cpu.a = a - b;
      SetZN(cpu.a);
      cpu.C = (a-b >= 0)?1:0;
      cpu.V = (((a^b) & 0x80) != 0 && ((a^cpu.a) & 0x80) != 0)?1:0; //TODO might be wrong
      break;
    }

    case 0x07:
    case 0x17:
    case 0x0F:
    case 0x1F:
    case 0x1B:
    case 0x03:
    case 0x13:{//SLO/ASO
      if(info.mode == accumulator){ //ASL
        cpu.C = (cpu.a >> 7) & 1;
        cpu.a <<= 1;
        SetZN(cpu.a);
      }
      else{
        value = cread(info.address);
        cpu.C = (value >> 7) & 1;
        value <<= 1;
        cwrite(info.address, value);
        SetZN(value);
      }

      cpu.a = cpu.a | cread(info.address); //ORA
      SetZN(cpu.a);
      break;
    }

    case 0x27:
    case 0x37:
    case 0x2F:
    case 0x3F:
    case 0x3B:
    case 0x23:
    case 0x33:{//RLA
      if(info.mode == accumulator){ //ROL
        c = cpu.C;
        cpu.C = (cpu.a >> 7) & 1;
        cpu.a = (cpu.a << 1) | c;
        SetZN(cpu.a);
      }
      else{
        c = cpu.C;
        value = cread(info.address);
        cpu.C = (value >> 7) & 1;
        value = (value << 1) | c;
        cwrite(info.address,value);
        SetZN(value);
      }

      cpu.a = cpu.a & cread(info.address); //AND
      SetZN(cpu.a);
      break;
    }

    case 0x4F:
    case 0x5F:
    case 0x5B:
    case 0x47:
    case 0x57:
    case 0x43:
    case 0x53:{//SRE
      if(info.mode == accumulator){ //LSR
        cpu.C = cpu.a & 1;
        cpu.a >>= 1;
        SetZN(cpu.a);
      }
      else{
        value = cread(info.address);
        cpu.C = value & 1;
        value >>=1;
        cwrite(info.address,value);
        SetZN(value);
      }

      cpu.a = cpu.a^cread(info.address); //EOR
      SetZN(cpu.a);
      break;
    }

    case 0x6F:
    case 0x7F:
    case 0x7B:
    case 0x67:
    case 0x77:
    case 0x63:
    case 0x73:{//RRA
      if(info.mode == accumulator){ //ROR
        c = cpu.C;
        cpu.C = cpu.a & 1;
        cpu.a = (cpu.a >> 1) | (c << 7);
        SetZN(cpu.a);
      }
      else{
        c = cpu.C;
        value = cread(info.address);
        cpu.C = value & 1;
        value = (value >> 1) | (c << 7);
        cwrite(info.address,value);
        SetZN(value);
      }

      a = cpu.a; //ADC
      b = cread(info.address);
      c = cpu.C;
      cpu.a = a+b+c;
      SetZN(cpu.a);
      if(a+b+c > 0xFF) cpu.C = 1;
      else cpu.C = 0;
      if( ((a^b)&0x80) == 0 && ((a^cpu.a)&0x80) != 0) cpu.V = 1;
      else cpu.V = 0;
      break;
    }

    default:
    //un-official opcodes
    //  ahx
    //  alr
    //  anc
    //  arr
    //  axs
    //  dcp // remove
    //  isc // remove
    //  kil
    //  las
    //  lax // remove
    //  rla // remove
    //  rra // remove
    //  sax // remove
    //  shx
    //  shy
    //  slo // remove
    //  sre // remove
    //  tas
    //  xaa
    break;
  }
}

void NMI(){
  struct stepInfo info = {0,0};
  push16(cpu.pc);
  execute(0x08,info); //php
  cpu.pc = cread16(0xFFFA);
  cpu.I = 1;
  cpu.cycles += 7;
}
void IRQ(){
  struct stepInfo info = {0,0};
  push16(cpu.pc);
  execute(0x08,info); //php
  cpu.pc = cread16(0xFFFE);
  cpu.I = 1;
  cpu.cycles += 7;
}
void check_interrupt(){
  switch(cpu.interrupt){
    case int_NMI: NMI(); break;
    case int_IRQ: IRQ(); break;
  }
}

char line[500];
void logger(){
  u8 opcode = cread(cpu.pc+0);
  u8 operand1 = cread(cpu.pc+1);
  u8 operand2 = cread(cpu.pc+2);
  u8 flags = GetFlags();
  u8 mode = inst_modes[opcode];
  u8 inst_cyc = inst_cycles[opcode];
  if(inst_size[opcode]==3){

  }else if(inst_size[opcode]==2){
    operand2 = 0xAF;
  }else if(inst_size[opcode]==1){
    operand1 = 0xAF;
    operand2 = 0xAF;
  }
  switch(mode){
    case absolute:
    case absolute_X:
    case absolute_Y:
    case indexed_indirect:
    case indirect:
    case indirect_indexed:
    case relative:
    case zero_page:
    case zero_page_X:
    case zero_page_Y:
      sprintf(line,"%x\t%-2x %-2x %-2x @%-4x->%-4x A:%-2x X:%-2x Y:%-2x P:%-2x SP:%-4x CYC:%d\tM:%d\tIC:%d\ti_cyc:%d\t$$PPU_CYC:%d\t$$Scanline:%d\tFrame:%d\n",cpu.pc,opcode,operand1,operand2,cpu.fetch_address,main_mem[cpu.fetch_address],cpu.a,cpu.x,cpu.y,flags,cpu.sp,cpu.cycles,mode,cpu.inst_count,inst_cyc,ppu.cycles,ppu.ScanLine,ppu.Frame);
      break;
    default:
      sprintf(line,"%x\t%-2x %-2x %-14x A:%-2x X:%-2x Y:%-2x P:%-2x SP:%-4x CYC:%d\tM:%d\tIC:%d\ti_cyc:%d\t$$PPU_CYC:%d\t$$Scanline:%d\tFrame:%d\n",cpu.pc,opcode,operand1,operand2,cpu.a,cpu.x,cpu.y,flags,cpu.sp,cpu.cycles,mode,cpu.inst_count,inst_cyc,ppu.cycles,ppu.ScanLine,ppu.Frame);
  }
  printf("%s",line);
  //fprintf(log_file,"%s",line);
}

uint cStep(){
  uint cycles = cpu.cycles;
  if(cpu.delay == true){
    return 1;
  }

  u8 opcode = cread(cpu.pc);
  cpu.opcode = opcode;
  cpu.opcode_address = cpu.pc;

  uint mode = inst_modes[opcode];
  u16 address = 0;
  uint page_crossed = false;

  mode_addressing(mode,&address,&page_crossed);
  logger();
  check_interrupt();
  if(cpu.interrupt!=int_none){
    cpu.interrupt = int_none;
    return cpu.cycles - cycles;
  }
  cpu.interrupt = int_none;
  cpu.inst_count++;
  cpu.pc += inst_size[opcode];
  cpu.cycles += inst_cycles[opcode];
  if(page_crossed == 1) cpu.cycles += inst_Page_cycles[opcode];
  struct stepInfo info = {address,cpu.pc,mode};
  execute(opcode,info);

  return cpu.cycles - cycles;
}

void cpu_reset(){
  cpu.delay = false;
  cpu.inst_count = 1;
  cpu.terminate = false;
  cpu.cycles = 7;
  cpu.pc = cread16(0xFFFC);
  //cpu.pc = 0xC000;
  cpu.sp = 0xFD;
  cpu.a = 0x00;
  cpu.x = 0x00;
  cpu.y = 0x00;
  SetFlags(0x24);
}
