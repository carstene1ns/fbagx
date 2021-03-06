// FB Alpha D-Con / SD Gundam Psycho Salamander no Kyoui driver module
// Based on MAME driver by Bryan McPhail

#include "tiles_generic.h"
#include "seibusnd.h"

static unsigned char *AllMem;
static unsigned char *RamEnd;
static unsigned char *MemEnd;
static unsigned char *AllRam;
static unsigned char *Drv68KROM;
static unsigned char *DrvZ80ROM;
static unsigned char *DrvGfxROM0;
static unsigned char *DrvGfxROM1;
static unsigned char *DrvGfxROM2;
static unsigned char *DrvGfxROM3;
static unsigned char *DrvGfxROM4;
static unsigned char *DrvSndROM;
static unsigned char *Drv68KRAM;
static unsigned char *DrvPalRAM;
static unsigned char *DrvSprRAM;
static unsigned char *DrvBgRAM;
static unsigned char *DrvMgRAM;
static unsigned char *DrvFgRAM;
static unsigned char *DrvTxRAM;
static unsigned char *DrvZ80RAM;

static unsigned int *DrvPalette;
static unsigned char DrvRecalc;

static unsigned short *scroll;

static int gfx_bank;
static int gfx_enable;

static unsigned char DrvJoy1[16];
static unsigned char DrvJoy2[16];
static unsigned char DrvJoy3[16];
static unsigned char DrvDips[2];
static unsigned short DrvInputs[3];
static unsigned char DrvReset;

static int is_sdgndmps = 0;

static struct BurnInputInfo DconInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy3 + 0,	"p1 start"	},
	{"P1 Up",		BIT_DIGITAL,	DrvJoy2 + 0,	"p1 up"		},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy2 + 1,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy2 + 2,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 3,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy2 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy2 + 5,	"p1 fire 2"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy1 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy3 + 4,	"p2 start"	},
	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 8,	"p2 up"		},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 9,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 10,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 11,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy2 + 12,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy2 + 13,	"p2 fire 2"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"		},
	{"Service",		BIT_DIGITAL,	DrvJoy3 + 12,	"service"	},
	{"Dip A",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",		BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
};

STDINPUTINFO(Dcon)

static struct BurnDIPInfo DconDIPList[]=
{
	{0x12, 0xff, 0xff, 0xff, NULL			},
	{0x13, 0xff, 0xff, 0xff, NULL			},

	{0   , 0xfe, 0   ,    8, "Coin A"		},
	{0x12, 0x01, 0x07, 0x00, "5 Coins 1 Credits"	},
	{0x12, 0x01, 0x07, 0x01, "4 Coins 1 Credits"	},
	{0x12, 0x01, 0x07, 0x02, "3 Coins 1 Credits"	},
	{0x12, 0x01, 0x07, 0x03, "2 Coins 1 Credits"	},
	{0x12, 0x01, 0x07, 0x07, "1 Coin  1 Credits"	},
	{0x12, 0x01, 0x07, 0x06, "1 Coin  2 Credits"	},
	{0x12, 0x01, 0x07, 0x05, "1 Coin  3 Credits"	},
	{0x12, 0x01, 0x07, 0x04, "1 Coin  4 Credits"	},

	{0   , 0xfe, 0   ,    8, "Coin B"		},
	{0x12, 0x01, 0x38, 0x00, "5 Coins 1 Credits"	},
	{0x12, 0x01, 0x38, 0x08, "4 Coins 1 Credits"	},
	{0x12, 0x01, 0x38, 0x10, "3 Coins 1 Credits"	},
	{0x12, 0x01, 0x38, 0x18, "2 Coins 1 Credits"	},
	{0x12, 0x01, 0x38, 0x38, "1 Coin  1 Credits"	},
	{0x12, 0x01, 0x38, 0x30, "1 Coin  2 Credits"	},
	{0x12, 0x01, 0x38, 0x28, "1 Coin  3 Credits"	},
	{0x12, 0x01, 0x38, 0x20, "1 Coin  4 Credits"	},
};

STDDIPINFO(Dcon)

static struct BurnDIPInfo SdgndmpsDIPList[]=
{
	{0x12, 0xff, 0xff, 0xff, NULL			},
	{0x13, 0xff, 0xff, 0xdf, NULL			},

	{0   , 0xfe, 0   ,   16, "Coin B"		},
	{0x12, 0x01, 0x0f, 0x04, "4 Coins 1 Credits"	},
	{0x12, 0x01, 0x0f, 0x0a, "3 Coins 1 Credits"	},
	{0x12, 0x01, 0x0f, 0x01, "2 Coins 1 Credits"	},
	{0x12, 0x01, 0x0f, 0x00, "5 Coins 3 Credits"	},
	{0x12, 0x01, 0x0f, 0x02, "3 Coins 2 Credits"	},
	{0x12, 0x01, 0x0f, 0x08, "4 Coins 3 Credits"	},
	{0x12, 0x01, 0x0f, 0x0f, "1 Coin  1 Credits"	},
	{0x12, 0x01, 0x0f, 0x0c, "3 Coins 4 Credits"	},
	{0x12, 0x01, 0x0f, 0x0e, "2 Coins 3 Credits"	},
	{0x12, 0x01, 0x0f, 0x07, "1 Coin  2 Credits"	},
	{0x12, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"	},
	{0x12, 0x01, 0x0f, 0x0b, "1 Coin  3 Credits"	},
	{0x12, 0x01, 0x0f, 0x03, "1 Coin  4 Credits"	},
	{0x12, 0x01, 0x0f, 0x0d, "1 Coin  5 Credits"	},
	{0x12, 0x01, 0x0f, 0x05, "1 Coin  6 Credits"	},
	{0x12, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"	},

	{0   , 0xfe, 0   ,   16, "Coin A"		},
	{0x12, 0x01, 0xf0, 0x40, "4 Coins 1 Credits"	},
	{0x12, 0x01, 0xf0, 0xa0, "3 Coins 1 Credits"	},
	{0x12, 0x01, 0xf0, 0x10, "2 Coins 1 Credits"	},
	{0x12, 0x01, 0xf0, 0x20, "3 Coins 2 Credits"	},
	{0x12, 0x01, 0xf0, 0x80, "4 Coins 3 Credits"	},
	{0x12, 0x01, 0xf0, 0xf0, "1 Coin  1 Credits"	},
	{0x12, 0x01, 0xf0, 0xc0, "3 Coins 4 Credits"	},
	{0x12, 0x01, 0xf0, 0xe0, "2 Coins 3 Credits"	},
	{0x12, 0x01, 0xf0, 0x70, "1 Coin  2 Credits"	},
	{0x12, 0x01, 0xf0, 0x60, "2 Coins 5 Credits"	},
	{0x12, 0x01, 0xf0, 0xb0, "1 Coin  3 Credits"	},
	{0x12, 0x01, 0xf0, 0x30, "1 Coin  4 Credits"	},
	{0x12, 0x01, 0xf0, 0xd0, "1 Coin  5 Credits"	},
	{0x12, 0x01, 0xf0, 0x50, "1 Coin  6 Credits"	},
	{0x12, 0x01, 0xf0, 0x90, "1 Coin  7 Credits"	},
	{0x12, 0x01, 0xf0, 0x00, "Free Play"		},

	{0   , 0xfe, 0   ,    4, "Lives"		},
	{0x13, 0x01, 0x03, 0x02, "2"			},
	{0x13, 0x01, 0x03, 0x03, "3"			},
	{0x13, 0x01, 0x03, 0x01, "4"			},
	{0x13, 0x01, 0x03, 0x00, "6"			},

	{0   , 0xfe, 0   ,    2, "Allow Continue"	},
	{0x13, 0x01, 0x20, 0x20, "No"			},
	{0x13, 0x01, 0x20, 0x00, "Yes"			},

};

STDDIPINFO(Sdgndmps)

void __fastcall dcon_main_write_word(unsigned int address, unsigned short data)
{
	if ((address & 0xfffff0) == 0x0a0000) {
		seibu_main_word_write(address, data);
		return;
	}

	if ((address & 0xfff800) == 0x09d000) {
		gfx_bank = (data & 1) << 12;
		return;
	}

	if ((address & 0xfffff0) == 0x0c0020) {
		scroll[(address & 0x0f) >> 1] = data;
		return;
	}

	if (address == 0x0c001c) {
		gfx_enable = data;
		return;
	}
}

unsigned short __fastcall dcon_main_read_word(unsigned int address)
{
	if ((address & 0xfffff0) == 0x0a0000) {
		return seibu_main_word_read(address);
	}

	switch (address)
	{
		case 0x0e0000:
			return (DrvDips[1] << 8) | DrvDips[0];

		case 0x0e0002:
			return DrvInputs[1];

		case 0x0e0004:
			return DrvInputs[2];
	}

	if (address == 0x0c001c) {
		return gfx_enable;
	}

	return 0;
}

static int DrvGfxDecode()
{
	int Plane0[4]  = { 0x00000, 0x00004, 0x80000, 0x80004 };
	int XOffs0[8]  = { 0x003, 0x002, 0x001, 0x000, 0x00b, 0x00a, 0x009, 0x008 };
	int YOffs0[8]  = { 0x000, 0x010, 0x020, 0x030, 0x040, 0x050, 0x060, 0x070 };

	int Plane1[4]  = { 0x008, 0x00c, 0x000, 0x004 };
	int XOffs1[16] = { 0x003, 0x002, 0x001, 0x000, 0x013, 0x012, 0x011, 0x010,
			   0x203, 0x202, 0x201, 0x200, 0x213, 0x212, 0x211, 0x210 };
	int YOffs1[16] = { 0x000, 0x020, 0x040, 0x060, 0x080, 0x0a0, 0x0c0, 0x0e0,
			   0x100, 0x120, 0x140, 0x160, 0x180, 0x1a0, 0x1c0, 0x1e0 };

	unsigned char *tmp = (unsigned char*)malloc(0x200000);
	if (tmp == NULL) {
		return 1;
	}

	memcpy (tmp, DrvGfxROM0, 0x020000);

	GfxDecode(0x1000, 4,  8,  8, Plane0, XOffs0, YOffs0, 0x080, tmp, DrvGfxROM0);

	memcpy (tmp, DrvGfxROM1, 0x80000);

	GfxDecode(0x1000, 4, 16, 16, Plane1, XOffs1, YOffs1, 0x400, tmp, DrvGfxROM1);

	memcpy (tmp, DrvGfxROM2, 0x80000);

	GfxDecode(0x1000, 4, 16, 16, Plane1, XOffs1, YOffs1, 0x400, tmp, DrvGfxROM2);

	memcpy (tmp, DrvGfxROM3, 0x100000);

	GfxDecode(0x2000, 4, 16, 16, Plane1, XOffs1, YOffs1, 0x400, tmp, DrvGfxROM3);

	memcpy (tmp, DrvGfxROM4, 0x200000);

	GfxDecode(0x4000, 4, 16, 16, Plane1, XOffs1, YOffs1, 0x400, tmp, DrvGfxROM4);

	free (tmp);

	return 0;
}

static int DrvDoReset()
{
	DrvReset = 0;

	memset (AllRam, 0, RamEnd - AllRam);

	SekOpen(0);
	SekReset();
	SekClose();

	seibu_sound_reset();

	gfx_bank = 0;
	gfx_enable = 0;

	return 0;
}

static int MemIndex()
{
	unsigned char *Next; Next = AllMem;

	Drv68KROM	= Next; Next += 0x080000;
	SeibuZ80ROM	= Next;
	DrvZ80ROM	= Next; Next += 0x020000;

	DrvGfxROM0	= Next; Next += 0x040000;
	DrvGfxROM1	= Next; Next += 0x100000;
	DrvGfxROM2	= Next; Next += 0x100000;
	DrvGfxROM3	= Next; Next += 0x200000;
	DrvGfxROM4	= Next; Next += 0x400000;

	DrvPalette	= (unsigned int*)Next; Next += 0x0800 * sizeof(int);

	MSM6295ROM	= Next;
	DrvSndROM	= Next; Next += 0x040000;

	AllRam		= Next;

	DrvSprRAM	= Next; Next += 0x000800;
	DrvBgRAM	= Next; Next += 0x000800;
	DrvMgRAM	= Next; Next += 0x000800;
	DrvFgRAM	= Next; Next += 0x000800;
	DrvTxRAM	= Next; Next += 0x001000;
	Drv68KRAM	= Next; Next += 0x00c000;
	DrvPalRAM	= Next; Next += 0x001000;

	scroll		= (unsigned short*)Next; Next += 0x000010;

	SeibuZ80RAM	= Next;
	DrvZ80RAM	= Next; Next += 0x000800;

	RamEnd		= Next;

	MemEnd		= Next;

	return 0;
}

static int DrvInit()
{
	AllMem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((AllMem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	{
		if (BurnLoadRom(Drv68KROM  + 0x000001,  0, 2)) return 1;
		if (BurnLoadRom(Drv68KROM  + 0x000000,  1, 2)) return 1;
		if (BurnLoadRom(Drv68KROM  + 0x040001,  2, 2)) return 1;
		if (BurnLoadRom(Drv68KROM  + 0x040000,  3, 2)) return 1;

		if (BurnLoadRom(DrvZ80ROM  + 0x000000,  4, 1)) return 1;
		memcpy (DrvZ80ROM + 0x10000, DrvZ80ROM + 0x08000, 0x08000);
		memcpy (DrvZ80ROM + 0x18000, DrvZ80ROM + 0x08000, 0x08000);

		if (BurnLoadRom(DrvGfxROM0 + 0x000000,  5, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM0 + 0x010000,  6, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x000000,  7, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM2 + 0x000000,  8, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM3 + 0x000000,  9, 1)) return 1;

		if (!strcmp(BurnDrvGetTextA(DRV_NAME), "sdgndmps")) {
			if (BurnLoadRom(DrvGfxROM4 + 0x000000, 10, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM4 + 0x100000, 11, 1)) return 1;
	
			if (BurnLoadRom(DrvSndROM  + 0x000000, 12, 1)) return 1;

			is_sdgndmps = 1;
		} else {
			if (BurnLoadRom(DrvGfxROM4 + 0x000000, 10, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM4 + 0x080000, 11, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM4 + 0x100000, 12, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM4 + 0x180000, 13, 1)) return 1;
	
			if (BurnLoadRom(DrvSndROM  + 0x000000, 14, 1)) return 1;
		}

		DrvGfxDecode();
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x07ffff, SM_ROM);
	SekMapMemory(Drv68KRAM,		0x080000, 0x08bfff, SM_RAM);
	SekMapMemory(DrvBgRAM,		0x08c000, 0x08c7ff, SM_RAM);
	SekMapMemory(DrvFgRAM,		0x08c800, 0x08cfff, SM_RAM);
	SekMapMemory(DrvMgRAM,		0x08d000, 0x08d7ff, SM_RAM);
	SekMapMemory(DrvTxRAM,		0x08d800, 0x08e7ff, SM_RAM);
	SekMapMemory(DrvPalRAM,		0x08e800, 0x08f7ff, SM_RAM);
	SekMapMemory(DrvSprRAM,		0x08f800, 0x08ffff, SM_RAM);
	SekSetWriteWordHandler(0,	dcon_main_write_word);
	SekSetReadWordHandler(0,	dcon_main_read_word);
	SekClose();

	seibu_sound_init(is_sdgndmps, 0, 3579545, 3579545, 1320000/132);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	GenericTilesExit();
	SekExit();

	seibu_sound_exit();

	free (AllMem);
	AllMem = NULL;

	is_sdgndmps = 0;

	return 0;
}

static void draw_txt_layer(int scrollx, int scrolly)
{
	unsigned short *vram = (unsigned short*)DrvTxRAM;

	for (int offs = 0; offs < 64 * 32; offs++)
	{
		int sx = (offs & 0x3f) << 3;
		int sy = (offs >> 6) << 3;

		sx -= scrollx;
		if (sx < -7) sx += 512;
		sy -= scrolly;
		if (sy < -7) sy += 256;

		if (sx >= nScreenWidth || sy >= nScreenHeight) continue;

		int code = vram[offs];
		int color = code >> 12;
		code &= 0xfff;

		Render8x8Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 0x0f, 0x700, DrvGfxROM0);
	}
}

static void draw_layer(unsigned char *ram, unsigned char *gfx, int col, int scrollx, int scrolly, int trans, int bank)
{
	scrollx &= 0x1ff;
	scrolly &= 0x1ff;
	
	unsigned short *vram = (unsigned short*)ram;

	for (int offs = 0; offs < 32 * 32; offs++)
	{
		int sx = (offs & 0x1f) << 4;
		int sy = (offs >> 5) << 4;

		sx -= scrollx;
		if (sx < -15) sx += 512;
		sy -= scrolly;
		if (sy < -15) sy += 512;

		int code = vram[offs];
		int color = (code >> 12) & 0x0f;
		code &= 0xfff;
		code |= bank;

		if (sy >= nScreenHeight || sx >= nScreenWidth) continue;

		if (trans) {
			Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 0x0f, col, gfx);
		} else {
			Render16x16Tile_Clip(pTransDraw, code, sx, sy, color, 4, col, gfx);
		}
	}
}

static inline void draw_sprite(int code, int color, int sx, int sy, int fx, int fy)
{
	if (sx < -15 || sy < -15 || sx >= nScreenWidth || sy >= nScreenHeight) return;

	if (fy) {
		if (fx) {
			Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, code, sx, sy, color, 4, 0xf, 0, DrvGfxROM4);
		} else {
			Render16x16Tile_Mask_FlipY_Clip(pTransDraw, code, sx, sy, color, 4, 0xf, 0, DrvGfxROM4);
		}
	} else {
		if (fx) {
			Render16x16Tile_Mask_FlipX_Clip(pTransDraw, code, sx, sy, color, 4, 0xf, 0, DrvGfxROM4);
		} else {
			Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 0xf, 0, DrvGfxROM4);
		}
	}
}

static void draw_sprites(int priority, int yoffset)
{
	unsigned short *ram = (unsigned short*)DrvSprRAM;

	for (int offs = 0x400; offs >= 0; offs -= 4)
	{
		int attr = ram[offs];
		if (~attr & 0x8000) continue;

		int sprite = ram[offs+1];

		int prio = (sprite >> 14) & 3;
		if (prio != priority) continue;

		sprite &= 0x3fff;

		int y = ram[offs+3];
		int x = ram[offs+2];

		if (x & 0x8000) x = 0 - (0x200 - (x & 0x1ff));
		else x &= 0x1ff;
		if (y & 0x8000) y = 0 - (0x200 - (y & 0x1ff));
		else y &= 0x1ff;

		int color = attr & 0x003f;
		int fx    = attr & 0x4000;
		int fy    = attr & 0x2000;
		int dy    = ((attr & 0x0380) >>  7) + 1;
		int dx    = ((attr & 0x1c00) >> 10) + 1;

		y -= yoffset;

		for (int ax = 0; ax < dx; ax++)
		{
			for (int ay = 0; ay < dy; ay++)
			{
				if (!fx && !fy)
				{
					draw_sprite(sprite, color, x+ax*16, y+ay*16, fx, fy);
					draw_sprite(sprite, color, x+ax*16, y+ay*16 + 512, fx, fy);
					draw_sprite(sprite, color, x+ax*16, y+ay*16 - 512, fx, fy);
				}
				else if (fx && !fy)
				{
					draw_sprite(sprite, color, x+(dx-1-ax)*16, y+ay*16, fx, fy);
					draw_sprite(sprite, color, x+(dx-1-ax)*16, y+ay*16 + 512, fx, fy);
					draw_sprite(sprite, color, x+(dx-1-ax)*16, y+ay*16 - 512, fx, fy);
				}
				else if (!fx && fy)
				{
					draw_sprite(sprite, color, x+ax*16, y+(dy-1-ay)*16, fx, fy);
					draw_sprite(sprite, color, x+ax*16, y+(dy-1-ay)*16 + 512, fx, fy);
					draw_sprite(sprite, color, x+ax*16, y+(dy-1-ay)*16 - 512, fx, fy);
				}
				else
				{
					draw_sprite(sprite, color, x+(dx-1-ax)*16, y+(dy-1-ay)*16, fx, fy);
					draw_sprite(sprite, color, x+(dx-1-ax)*16, y+(dy-1-ay)*16 + 512, fx, fy);
					draw_sprite(sprite, color, x+(dx-1-ax)*16, y+(dy-1-ay)*16 - 512, fx, fy);
				}

				sprite++;
				sprite &= 0x3fff;
			}
		}
	}
}

static inline void DrvRecalcPalette()
{
	unsigned char r,g,b;
	unsigned short *p = (unsigned short*)DrvPalRAM;
	for (int i = 0; i < 0x1000/2; i++) {
		int d = p[i];

		b = (d >> 10) & 0x1f;
		g = (d >>  5) & 0x1f;
		r = (d >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		DrvPalette[i] = BurnHighCol(r, g, b, 0);
	}
}

static void DrvDraw(int xoffset, int yoffset)
{
	if (DrvRecalc) {
		DrvRecalcPalette();
	}

	if (~gfx_enable & 1) {
		draw_layer(DrvBgRAM, DrvGfxROM1, 0x400, scroll[0] + xoffset, scroll[1] + yoffset, 0, 0);
	} else {
		for (int i = 0; i < nScreenWidth * nScreenHeight; i++)
			pTransDraw[i] = 0x0f;
	}

	draw_sprites(2, yoffset);

	if (~gfx_enable & 2) draw_layer(DrvMgRAM, DrvGfxROM3, 0x500, scroll[2] + xoffset, scroll[3] + yoffset, 1, gfx_bank);

	draw_sprites(1, yoffset);

	if (~gfx_enable & 4) draw_layer(DrvFgRAM, DrvGfxROM2, 0x600, scroll[4] + xoffset, scroll[5] + yoffset, 1, 0);

	draw_sprites(0, yoffset);

	draw_txt_layer(xoffset, yoffset);

	draw_sprites(3, yoffset);

	BurnTransferCopy(DrvPalette);
}

static int DconDraw()
{
	DrvDraw(0, 0);

	return 0;
}

static int SdgndmpsDraw()
{
	DrvDraw(128, 16);

	return 0;
}

static int DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	ZetNewFrame();

	{
		memset (DrvInputs, 0xff, 3 * 3);
		for (int i = 0; i < 16; i++)
		{
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
			DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
		}

		seibu_coin_input = (DrvJoy1[1] << 1) | DrvJoy1[0];
	}

	int nSegment;
	int nInterleave = 1000;
	int nTotalCycles[2] = { 10000000 / 60, 3579545 / 60 };
	int nCyclesDone[2] = { 0, 0 };

	SekOpen(0);
	ZetOpen(0);

	if (is_sdgndmps)
	{
		for (int i = 0; i < nInterleave; i++)
		{
			SekRun(nTotalCycles[0] / nInterleave);
			ZetRun(nTotalCycles[1] / nInterleave);
		}
	}
	else
	{
		for (int i = 0; i < nInterleave; i++)
		{
			nSegment = nTotalCycles[0] / nInterleave;

			nCyclesDone[0] += SekRun(nSegment);

			BurnTimerUpdate(nTotalCycles[1] / nInterleave);
		}

		BurnTimerEndFrame(nTotalCycles[1]);
	}

	SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);

	if (pBurnSoundOut) {
		seibu_sound_update(pBurnSoundOut, nBurnSoundLen);
	}

	ZetClose();
	SekClose();

	if (pBurnDraw) {
		BurnDrvRedraw();
	}

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {
		*pnMin = 0x029706;
	}

	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd-AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		SekScan(nAction);

		seibu_sound_scan(pnMin, nAction);

		SCAN_VAR(gfx_bank);
		SCAN_VAR(gfx_enable);
	}

	return 0;
}


// D-Con

static struct BurnRomInfo dconRomDesc[] = {
	{ "p0-0",	 0x020000, 0xa767ec15, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code
	{ "p0-1",	 0x020000, 0xa7efa091, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "p1-0",	 0x020000, 0x3ec1ef7d, 1 | BRF_PRG | BRF_ESS }, //  2
	{ "p1-1",	 0x020000, 0x4b8de320, 1 | BRF_PRG | BRF_ESS }, //  3

	{ "fm",		 0x010000, 0x50450faa, 2 | BRF_PRG | BRF_ESS }, //  4 Z80 Code

	{ "fix0",	 0x010000, 0xab30061f, 3 | BRF_GRA },           //  5 Characters
	{ "fix1",	 0x010000, 0xa0582115, 3 | BRF_GRA },           //  6

	{ "bg1",	 0x080000, 0xeac43283, 4 | BRF_GRA },           //  7 Background Tiles

	{ "bg3",	 0x080000, 0x1408a1e0, 5 | BRF_GRA },           //  8 Foreground Tiles

	{ "bg2",	 0x080000, 0x01864eb6, 6 | BRF_GRA },           //  9 Midground Tiles

	{ "obj0",	 0x080000, 0xc3af37db, 7 | BRF_GRA },           // 10 Sprites
	{ "obj1",	 0x080000, 0xbe1f53ba, 7 | BRF_GRA },           // 11
	{ "obj2",	 0x080000, 0x24e0b51c, 7 | BRF_GRA },           // 12
	{ "obj3",	 0x080000, 0x5274f02d, 7 | BRF_GRA },           // 13

	{ "pcm",	 0x020000, 0xd2133b85, 8 | BRF_SND },           // 14 OKIM6295 Samples
};

STD_ROM_PICK(dcon)
STD_ROM_FN(dcon)

struct BurnDriver BurnDrvDcon = {
	"dcon", NULL, NULL, "1992",
	"D-Con\0", NULL, "Success", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SHOOT, 0,
	NULL, dconRomInfo, dconRomName, DconInputInfo, DconDIPInfo,
	DrvInit, DrvExit, DrvFrame, DconDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc, 0x800,
	320, 224, 4, 3
};


// SD Gundam Psycho Salamander no Kyoui

static struct BurnRomInfo sdgndmpsRomDesc[] = {
	{ "911-a01.25",	 0x020000, 0x3362915d, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code
	{ "911-a02.29",	 0x020000, 0xfbc78285, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "911-a03.27",	 0x020000, 0x6c24b4f2, 1 | BRF_PRG | BRF_ESS }, //  2
	{ "911-a04.28",	 0x020000, 0x6ff9d716, 1 | BRF_PRG | BRF_ESS }, //  3

	{ "911-a05.010", 0x010000, 0x90455406, 2 | BRF_PRG | BRF_ESS }, //  4 Z80 Code

	{ "911-a08.66",	 0x010000, 0xe7e04823, 3 | BRF_GRA },           //  5 Characters
	{ "911-a07.73",	 0x010000, 0x6f40d4a9, 3 | BRF_GRA },           //  6

	{ "911-a12.63",	 0x080000, 0x8976bbb6, 4 | BRF_GRA },           //  7 Background Tiles

	{ "911-a11.65",	 0x080000, 0x3f3b7810, 5 | BRF_GRA },           //  8 Foreground Tiles

	{ "911-a13.64",	 0x100000, 0xf38a584a, 6 | BRF_GRA },           //  9 Midground Tiles

	{ "911-a10.73",	 0x100000, 0x80e341fb, 7 | BRF_GRA },           // 10 Sprites
	{ "911-a09.74",	 0x100000, 0x98f34519, 7 | BRF_GRA },           // 11

	{ "911-a06.97",	 0x040000, 0x12c79440, 8 | BRF_SND },           // 12 OKIM6295 Samples

	{ "bnd-007.88",	 0x000200, 0x96f7646e, 0 | BRF_OPT },           // 13 Priority Prom (not used)
};

STD_ROM_PICK(sdgndmps)
STD_ROM_FN(sdgndmps)

static int SdgndmpsInit()
{
	int nRet = DrvInit();

	if (nRet == 0)
	{
		// Patch out protection?
		*((unsigned short *)(Drv68KROM + 0x04de)) = 0x4245;
		*((unsigned short *)(Drv68KROM + 0x04e0)) = 0x4e71;
		*((unsigned short *)(Drv68KROM + 0x04e2)) = 0x4e71;
		*((unsigned short *)(Drv68KROM + 0x1356)) = 0x4e71;
		*((unsigned short *)(Drv68KROM + 0x1358)) = 0x4e71;
	}

	return nRet;
}

struct BurnDriver BurnDrvSdgndmps = {
	"sdgndmps", NULL, NULL, "1991",
	"SD Gundam Psycho Salamander no Kyoui\0", NULL, "Banpresto / Bandai", "Miscellaneous",
	L"\u30AC\u30F3\u30C0\u30E0 \u30B5\u30A4\u30B3\u30B5\u30E9\u30DE\uF303\u30C0\u30FC\u306E\u8105\u5A01\0SD Gundam Psycho Salamander no Kyoui\0", NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_HORSHOOT, 0,
	NULL, sdgndmpsRomInfo, sdgndmpsRomName, DconInputInfo, SdgndmpsDIPInfo,
	SdgndmpsInit, DrvExit, DrvFrame, SdgndmpsDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc, 0x800,
	320, 224, 4, 3
};
