#include<commons.h>
extern void triggerNMI();
struct PPU ppu;
struct CPU cpu;

u8 pread(u16 address){
  switch(address){
    case 0x2000 ... 0x23FF:
    case 0x2400 ... 0x27FF:
    case 0x2800 ... 0x2BFF:
    case 0x2C00 ... 0x2FFF:
      return ppu_mem[address%0x2BFF];
    default:
      return ppu_mem[address];
  }
}
void pwrite(u16 address,u8 value){
  switch(address){
    case 0x2000 ... 0x23FF:
    case 0x2400 ... 0x27FF:
    case 0x2800 ... 0x2BFF:
    case 0x2C00 ... 0x2FFF:
      printf("DATA WRTITEN TO NAMETABLE:0x%x, data:0x%x\n",address,value);
      ppu_mem[address%0x2BFF] = value;
      break;
    default:
      ppu_mem[address] = value;
  }
}

void writeControl(u8 value) {// $2000: PPUCTRL
	ppu.flagNameTable = (value >> 0) & 3;
	ppu.flagIncrement = (value >> 2) & 1;
	ppu.flagSpriteTable = (value >> 3) & 1;
	ppu.flagBackgroundTable = (value >> 4) & 1;
	ppu.flagSpriteSize = (value >> 5) & 1;
	ppu.flagMasterSlave = (value >> 6) & 1;
	ppu.gen_nmi = ((value>>7)&1);

	// t: ....BA.. ........ = d: ......BA
	ppu.t = (ppu.t & 0xF3FF) | (((u16)value & 0x03) << 10);
}
void writeMask(u8 value) {// $2001: PPUMASK
	ppu.flagGrayscale = (value >> 0) & 1;
	ppu.flagShowLeftBackground = (value >> 1) & 1;
	ppu.flagShowLeftSprites = (value >> 2) & 1;
	ppu.flagShowBackground = (value >> 3) & 1;
	ppu.flagShowSprites = (value >> 4) & 1;
	ppu.flagRedTint = (value >> 5) & 1;
	ppu.flagGreenTint = (value >> 6) & 1;
	ppu.flagBlueTint = (value >> 7) & 1;
}
u8 readStatus(){// $2002: PPUSTATUS
  u8 result;
	result = ppu.Register & 0x1F;
	result |= ppu.flagSpriteOverflow << 5;
	result |= ppu.flagSpriteZeroHit << 6;
	if (ppu.vblank_flag) {
		result |= 1 << 7;
	}
	ppu.vblank_flag = false;
	// w:                   = 0
	ppu.w = 0;
	return result;
}

void writeOAMAddress(u8 value) {// $2003: OAMADDR
	ppu.oamAddress = value;
}
u8 readOAMData(){// $2004: OAMDATA (read)
	return 0;
}
void writeOAMData(u8 value) {// $2004: OAMDATA (write)
	ppu.oamAddress++;
}

void writeScroll(u8 value) {// $2005: PPUSCROLL
	if (ppu.w == 0) {
		// t: ........ ...HGFED = d: HGFED...
		// x:               CBA = d: .....CBA
		// w:                   = 1
		ppu.t = (ppu.t & 0xFFE0) | ((u16)value >> 3);
		ppu.x = value & 0x07;
		ppu.w = 1;
	} else {
		// t: .CBA..HG FED..... = d: HGFEDCBA
		// w:                   = 0
		ppu.t = (ppu.t & 0x8FFF) | (((u16)value & 0x07) << 12);
		ppu.t = (ppu.t & 0xFC1F) | (((u16)value & 0xF8) << 2);
		ppu.w = 0;
	}
}

void writeAddress(u8 value) {// $2006: PPUADDR
	if(ppu.w == 0) {
		// t: ..FEDCBA ........ = d: ..FEDCBA
		// t: .X...... ........ = 0
		// w:                   = 1
		ppu.t = (ppu.t & 0x80FF) | (((u16)value & 0x3F) << 8);
		ppu.w = 1;
	} else {
		// t: ........ HGFEDCBA = d: HGFEDCBA
		// v                    = t
		// w:                   = 0
		ppu.t = (ppu.t & 0xFF00) | (u16)value;
		ppu.v = ppu.t;
		ppu.w = 0;
	}
}
u8 readData(){// $2007: PPUDATA (read)
  u8 value;
	value = pread(ppu.v);
	// emulate buffered reads
	if (ppu.v%0x4000 < 0x3F00) {
		u8 buffered = ppu.bufferedData;
		ppu.bufferedData = value;
		value = buffered;
	} else {
		ppu.bufferedData = pread(ppu.v - 0x1000);
	}
	// increment address
	if (ppu.flagIncrement == 0) {
		ppu.v += 1;
	} else {
		ppu.v += 32;
	}
	return value;
}
void writeData(u8 value) {// $2007: PPUDATA (write)
	pwrite(ppu.v, value);
	if (ppu.flagIncrement == 0) {
		ppu.v += 1;
	} else {
		ppu.v += 32;
	}
}
void writeDMA(u8 value) {// $4014: OAMDMA
	//cpu := ppu.console.CPU;
	u16 address = (u16)value << 8;
	for (int i = 0; i < 256; i++) {
		ppu.oamAddress++;
		address++;
	}
	cpu.stall += 513;
	if (cpu.cycles%2 == 1) {
		cpu.stall++;
	}
}

u8 readRegs(u16 address){
	switch (address) {
	case 0x2002:
		return readStatus();
	case 0x2004:
		return readOAMData();
	case 0x2007:
		return readData();
	}
	return 0;
}
void writeRegs(u16 address, u8 value) {
	ppu.Register = value;
	switch (address) {
	case 0x2000:
		writeControl(value);break;
	case 0x2001:
		writeMask(value);break;
	case 0x2003:
		writeOAMAddress(value);break;
	case 0x2004:
		writeOAMData(value);break;
	case 0x2005:
		writeScroll(value);break;
	case 0x2006:
		writeAddress(value);break;
	case 0x2007:
		writeData(value);break;
	case 0x4014:
		writeDMA(value);break;
	}
}

void fetch_NTbyte(){
  u16 address = 0x2000 | (ppu.v & 0x0FFF);
  ppu.nameTableByte = pread(address);
}
void fetch_Low_Tile_byte(){
  uint fineY = (u16)(ppu.v >> 12)&7;
  uint table = ppu.flagBackgroundTable;
  u8 tile = ppu.nameTableByte;
  u16 address = 0x1000*table + tile*16 + fineY;
  ppu.low_Tile_byte = pread(address);
}
void fetch_High_Tile_byte(){
  uint fineY = (u16)(ppu.v >> 12)&7;
  uint table = ppu.flagBackgroundTable;
  u8 tile = ppu.nameTableByte;
  u16 address = 0x1000*table + tile*16 + fineY;
  ppu.high_Tile_byte = pread(address + 8);
}
void fetch_ATbyte(){
  u16 v = ppu.v;
  u16 address = 0x23C0 | (v&0x0C00) | ((v>>4)&0x38) | ((v>>2)&0x07);
  uint shift = ((v >> 4)&4) | (v&2);
  ppu.attribute_Table_byte = ((pread(address) >> shift) & 3) << 2;
}
void store_Tile_data(){
  u32 data;
  for(int i=0;i<8;i++){
    u8 a = ppu.attribute_Table_byte;
    u8 p1 = (ppu.low_Tile_byte & 0x80)>>7;
    u8 p2 = (ppu.high_Tile_byte & 0x80)>>6;
    ppu.low_Tile_byte <<=1;
    ppu.high_Tile_byte <<=1;
    data <<=4;
    data |= (u32)(a|p1|p2);
  }
  ppu.tileData |= (u64)(data);
}

void inc_scl(){
  ppu.ScanLine++;
  if(ppu.ScanLine >260) {
    ppu.ScanLine = -1;
  }
  ppu.cycles = 0;
}
void inc_cyc(){
  ppu.cycles++;
  if(ppu.cycles >340) {
    inc_scl();
  }
}

void check_scanline(){
  switch(ppu.ScanLine){ //scanlines
    case -1:{ //Pre-render line
      if(ppu.cycles == 0) ppu.Frame++;
      inc_cyc();
      break;
    }
    case 0 ... 239:{    // visible scanline
      switch(ppu.cycles){ //cycles
        case 0: break;
        case 1 ... 256:{
          switch(ppu.cycles%8){ //operation
            case 1: fetch_NTbyte();break;
            case 2: break;
            case 3: fetch_ATbyte();break;
            case 4: break;
            case 5: fetch_Low_Tile_byte();break;
            case 6: break;
            case 7: fetch_High_Tile_byte();break;
            case 0: store_Tile_data();break;
          }
        }
        case 257 ... 320:{
          switch(ppu.cycles%8){
            case 1: break;
            case 2: break;
            case 3: break;
            case 4: break;
            case 5: break;
            case 6: break;
            case 7: break;
            case 0: break;
          }
        }
        case 321 ... 336:{
          switch(ppu.cycles%8){
            case 1: break;
            case 2: break;
            case 3: break;
            case 4: break;
            case 5: break;
            case 6: break;
            case 7: break;
            case 0: break;
          }
        }
        case 337 ... 340:{
          switch(ppu.cycles%4){
            case 0: break;
            case 1: break;
            case 2: break;
            case 3: break;
            case 4: break;
          }
        }
      }
      inc_cyc();
      break;
    }
    case 240:{ //Post-render line
      inc_cyc();
      break;
    }
    case 241 ... 260:{ //vblank line
      if(ppu.ScanLine == 241){
        switch (ppu.cycles) {
          case 0:
            ppu.vblank_flag=true;
            if(ppu.gen_nmi==true && ppu.vblank_flag == true)
            triggerNMI();
            break;
          case 1:
            break;
        }
      }
      inc_cyc();
      break;
    }
  }
}

void pStep(){
  check_scanline();
}

void ppu_reset(){
  ppu.cycles = 0;
  ppu.ScanLine = 0;
  ppu.Frame = 0;
  writeControl(0);
  writeMask(0);
  writeOAMAddress(0);
}
