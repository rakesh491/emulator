#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned int uint;
typedef unsigned long ulong;
#define true 1
#define false 0

extern u8 main_mem[];
extern u8 ppu_mem[];
extern u8 rom_buffer[];
extern FILE *log_file;
extern FILE *rom;

struct Rom_header{
  char string[4];
  char rom_prg_nos; //Number of 16384 byte program rom pages
  char rom_chr_nos; //Number of 8192 byte character rom pages
  char bitfield1; /* bit 0     1 for vertical mirroring, 0 for horizontal mirroring.
                     bit 1     1 for battery-backed RAM at $6000-$7FFF.
                     bit 2     1 for a 512-byte trainer at $7000-$71FF.
                     bit 3     1 for a four-screen VRAM layout.
                     bit 4-7   Four lower bits of ROM Mapper Type. */
  char bitfield2; /* bit 0     1 for VS-System cartridges.
                     bit 1-3   Reserved, must be zeroes!
                     bit 4-7   Four higher bits of ROM Mapper Type.*/
  char unused[8]; // must be 0
};
extern struct Rom_header *header;

struct CPU{
  u8 opcode;
  u16 opcode_address;
  uint delay;
  uint inst_count;
  uint cycles;
  uint terminate;
  uint stall;
  u16 fetch_address;
  u16 pc;
  u8 sp;
  u8 a,x,y;
  u8 C,Z,I,D,B,U,V,N; //carry,zero,interrupt-disabled,decimal,break,unused,overflow,negative
  u8 interrupt;
};
extern struct CPU cpu;

struct PPU{
  uint cycles;               // 0-340
	int ScanLine;              // 0-261, 0-239=visible, 240=post, 241-260=vblank, 261=pre
	uint Frame;                // frame counter
  uint prev_nmi;
  uint nmiDelay;
  u8 nameTableByte;
  u8 low_Tile_byte;
  u8 high_Tile_byte;
  u8 attribute_Table_byte;
  u64 tileData;

	// PPU registers
	u16 v;                     // current vram address (15 bit)
	u16 t;                     // temporary vram address (15 bit)
	u8 x;                      // fine x scroll (3 bit)
	u8 w;                      // write toggle (1 bit)
	u8 f;                      // even/odd frame flag (1 bit)

	u8 Register;

	// $2000 PPUCTRL
	u8 flagNameTable;          // 0: $2000; 1: $2400; 2: $2800; 3: $2C00
	u8 flagIncrement;          // 0: add 1; 1: add 32
	u8 flagSpriteTable;        // 0: $0000; 1: $1000; ignored in 8x16 mode
	u8 flagBackgroundTable;    // 0: $0000; 1: $1000
	u8 flagSpriteSize;         // 0: 8x8; 1: 8x16
	u8 flagMasterSlave;        // 0: read EXT; 1: write EXT
  u8 gen_nmi;                // nmi output

	// $2001 PPUMASK
	u8 flagGrayscale;          // 0: color; 1: grayscale
	u8 flagShowLeftBackground; // 0: hide; 1: show
	u8 flagShowLeftSprites;    // 0: hide; 1: show
	u8 flagShowBackground;     // 0: hide; 1: show
	u8 flagShowSprites;        // 0: hide; 1: show
	u8 flagRedTint;            // 0: normal; 1: emphasized
	u8 flagGreenTint;          // 0: normal; 1: emphasized
	u8 flagBlueTint;           // 0: normal; 1: emphasized

	// $2002 PPUSTATUS
	u8 flagSpriteZeroHit;
	u8 flagSpriteOverflow;
  u8 vblank_flag;            //nmi occurred

	// $2003 OAMADDR
	u8 oamAddress;

	// $2007 PPUDATA
	u8 bufferedData;           // for buffered reads
};
extern struct PPU ppu;

extern u8 pread(u16 address);
extern void pwrite(u16 address,u8 value);
extern void pause();
