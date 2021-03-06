// FB Alpha Return of the Invaders driver module
// Based on MAME driver by Jarek Parchanski and Andrea Mazzoleni

#include "tiles_generic.h"
#include "bitswap.h"
#include "sn76496.h"

static unsigned char *AllMem;
static unsigned char *AllRam;
static unsigned char *RamEnd;
static unsigned char *MemEnd;
static unsigned char *DrvZ80ROM0;
static unsigned char *DrvZ80ROM1;
static unsigned char *DrvZ80ROM2;
static unsigned char *DrvMCUROM;
static unsigned char *DrvGfxROM0;
static unsigned char *DrvGfxROM1;
static unsigned char *DrvGfxROM2;
static unsigned char *DrvColPROM;
static unsigned char *DrvFgVRAM;
static unsigned char *DrvBgVRAM;
static unsigned char *DrvShareRAM;
static unsigned char *DrvZ80RAM2;

static unsigned short *DrvTileOfst;
static unsigned char  *DrvColTable;
static unsigned int   *Palette;
static unsigned int   *DrvPalette;
static unsigned char   DrvRecalc;

static unsigned char *coinlockout;
static unsigned char *soundlatch;
static unsigned char *soundlatch2;
static unsigned char *flipscreen;
static unsigned char *gfxbank;

static int enable_interrupt[2];
static int disable_cpu[2];

static int protdata_last;
static int protdata;

static int watchdog;

static unsigned char DrvJoy1[8];
static unsigned char DrvJoy2[8];
static unsigned char DrvJoy3[8];
static unsigned char DrvDips[3];
static unsigned char DrvInputs[3];
static unsigned char DrvReset;

static struct BurnInputInfo RetofinvInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy3 + 4,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy3 + 0,	"p1 start"	},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy1 + 3,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy1 + 7,	"p1 fire 1"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy3 + 5,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy3 + 1,	"p2 start"	},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 3,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy2 + 7,	"p2 fire 1"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"		},
	{"Service",		BIT_DIGITAL,	DrvJoy3 + 3,	"service"	},
	{"Tilt",		BIT_DIGITAL,	DrvJoy3 + 2,	"tilt"		},
	{"Dip A",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",		BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",		BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Retofinv)

static struct BurnDIPInfo RetofinvDIPList[]=
{
	{0x0d, 0xff, 0xff, 0x6f, NULL					},
	{0x0e, 0xff, 0xff, 0x00, NULL					},
	{0x0f, 0xff, 0xff, 0xff, NULL					},

	{0   , 0xfe, 0   ,    4, "Bonus Life"				},
	{0x0d, 0x01, 0x03, 0x03, "30k, 80k & every 80k"			},
	{0x0d, 0x01, 0x03, 0x02, "30k, 80k"				},
	{0x0d, 0x01, 0x03, 0x01, "30k"					},
	{0x0d, 0x01, 0x03, 0x00, "None"					},

	{0   , 0xfe, 0   ,    2, "Free Play"				},
	{0x0d, 0x01, 0x04, 0x04, "No"					},
	{0x0d, 0x01, 0x04, 0x00, "Yes"					},

	{0   , 0xfe, 0   ,    4, "Lives"				},
	{0x0d, 0x01, 0x18, 0x18, "1"					},
	{0x0d, 0x01, 0x18, 0x10, "2"					},
	{0x0d, 0x01, 0x18, 0x08, "3"					},
	{0x0d, 0x01, 0x18, 0x00, "5 (retofin2 4)"			},

	{0   , 0xfe, 0   ,    2, "Flip Screen"				},
	{0x0d, 0x01, 0x40, 0x40, "Off"					},
	{0x0d, 0x01, 0x40, 0x00, "On"					},

	{0   , 0xfe, 0   ,    2, "Cabinet"				},
	{0x0d, 0x01, 0x80, 0x00, "Upright"				},
	{0x0d, 0x01, 0x80, 0x80, "Cocktail"				},

	{0   , 0xfe, 0   ,    16, "Coin A"				},
	{0x0e, 0x01, 0x0f, 0x0f, "9 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x0e, "8 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x0d, "7 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x0c, "6 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x0b, "5 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x0a, "4 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x09, "3 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x08, "2 Coins 1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x00, "1 Coin  1 Credits"			},
	{0x0e, 0x01, 0x0f, 0x01, "1 Coin  2 Credits"			},
	{0x0e, 0x01, 0x0f, 0x02, "1 Coin  3 Credits"			},
	{0x0e, 0x01, 0x0f, 0x03, "1 Coin  4 Credits"			},
	{0x0e, 0x01, 0x0f, 0x04, "1 Coin  5 Credits"			},
	{0x0e, 0x01, 0x0f, 0x05, "1 Coin  6 Credits"			},
	{0x0e, 0x01, 0x0f, 0x06, "1 Coin  7 Credits"			},
	{0x0e, 0x01, 0x0f, 0x07, "1 Coin  8 Credits"			},

	{0   , 0xfe, 0   ,    16, "Coin B"				},
	{0x0e, 0x01, 0xf0, 0xf0, "9 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0xe0, "8 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0xd0, "7 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0xc0, "6 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0xb0, "5 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0xa0, "4 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0x90, "3 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0x80, "2 Coins 1 Credits"			},
	{0x0e, 0x01, 0xf0, 0x00, "1 Coin  1 Credits"			},
	{0x0e, 0x01, 0xf0, 0x10, "1 Coin  2 Credits"			},
	{0x0e, 0x01, 0xf0, 0x20, "1 Coin  3 Credits"			},
	{0x0e, 0x01, 0xf0, 0x30, "1 Coin  4 Credits"			},
	{0x0e, 0x01, 0xf0, 0x40, "1 Coin  5 Credits"			},
	{0x0e, 0x01, 0xf0, 0x50, "1 Coin  6 Credits"			},
	{0x0e, 0x01, 0xf0, 0x60, "1 Coin  7 Credits"			},
	{0x0e, 0x01, 0xf0, 0x70, "1 Coin  8 Credits"			},

	{0   , 0xfe, 0   ,    2, "Push Start to Skip Stage (Cheat)"	},
	{0x0f, 0x01, 0x01, 0x01, "Off"					},
	{0x0f, 0x01, 0x01, 0x00, "On"					},

	{0   , 0xfe, 0   ,    2, "Coin Per Play Display"		},
	{0x0f, 0x01, 0x10, 0x00, "No"					},
	{0x0f, 0x01, 0x10, 0x10, "Yes"					},

	{0   , 0xfe, 0   ,    2, "Year Display"				},
	{0x0f, 0x01, 0x20, 0x00, "No"					},
	{0x0f, 0x01, 0x20, 0x20, "Yes"					},

	{0   , 0xfe, 0   ,    2, "Invulnerability (Cheat)"		},
	{0x0f, 0x01, 0x40, 0x40, "Off"					},
	{0x0f, 0x01, 0x40, 0x00, "On"					},

	{0   , 0xfe, 0   ,    2, "Coinage"				},
	{0x0f, 0x01, 0x80, 0x80, "A and B"				},
	{0x0f, 0x01, 0x80, 0x00, "A only"				},
};

STDDIPINFO(Retofinv)

// Reset cpu and keep sync
static inline void retofinv_cpu_reset(int cpu, int data)
{
	int cycles = ZetTotalCycles();
	ZetClose();
	ZetOpen(cpu);
	cycles -= ZetTotalCycles();
	if (data) {
		ZetIdle(cycles - ZetTotalCycles());
	} else {
		ZetRun(cycles);
		ZetReset();
	}
	disable_cpu[cpu-1] = data;
	ZetClose();
	ZetOpen(0);
}

// MCU simulation from an ancient version of MAME
static void retofinv_protection_write(int data)
{
	if (protdata_last & 0x80)
	{
		protdata = data;
		protdata_last = 0;
		return;
	}

	switch (data)
	{
		case 0x01:
			protdata = ((protdata >> 3) & 3) + 1;
		break;

		case 0x02:
			protdata &= 3;
		break;

		case 0x30:
		break;

		case 0x40:
		case 0x41:
		case 0x42:
			protdata = ZetReadByte(0x9800 + (data & 3));
		break;

		default:
			if (data > 0x0f) protdata = 0x3b;
		break;
	}

	protdata_last = data;
}

void __fastcall retofinv_main_write(unsigned short address, unsigned char data)
{
	switch (address)
	{
		case 0xb800:
			*flipscreen = data & 1;
		return;

		case 0xb801:
			gfxbank[0] = data & 1;
		return;

		case 0xb802:
			gfxbank[1] = data & 1;
		return;

		case 0xc800:
		{
			data &= 1;
			if (!data) ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
			enable_interrupt[0] = data;
		}
		return;
		
		case 0xc801:
			*coinlockout = (data & 1) * 0xff;
		return;

		case 0xc802:
			retofinv_cpu_reset(2, data);
		return;

		case 0xc803: // mcu reset
			protdata_last = 0;
			protdata = 0;
		return;

		case 0xc804:
		{
			data &= 1;
			if (!data) ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
			enable_interrupt[1] = data;
		}
		return;

		case 0xc805:
			retofinv_cpu_reset(1, data);
		return;

		case 0xd000:
			watchdog = 0;
		return;

		case 0xd800:
		{
			*soundlatch = data;
			ZetClose();
			ZetOpen(2);
			ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
			ZetClose();
			ZetOpen(0);
		}
		return;

		case 0xe800: // mcu write
			retofinv_protection_write(data);
		return;
	}
}

unsigned char __fastcall retofinv_main_read(unsigned short address)
{
	switch (address)
	{
		case 0xc000:
			return DrvInputs[0];

		case 0xc001:
			return DrvInputs[1];

		case 0xc002:	// must be < 0x80;
			return 0;

		case 0xc003:	// mcu status
			return 0x30;

		case 0xc004:
			return DrvInputs[2] & (*coinlockout | 0xcf);

		case 0xc005:
			return DrvDips[0];

		case 0xc006:
			return DrvDips[1];

		case 0xc007:
			return DrvDips[2];

		case 0xf800:
			return *soundlatch2;

		case 0xe000:	// mcu read
			return protdata; 
	}

	return 0;
}

void __fastcall retofinv_sound_write(unsigned short address, unsigned char data)
{
	switch (address)
	{
		case 0x6000:
			*soundlatch2 = data;
		return;

		case 0x8000:
			SN76496Write(0, data);
		return;

		case 0xa000:
			SN76496Write(1, data);
		return;
	}
}

unsigned char __fastcall retofinv_sound_read(unsigned short address)
{
	switch (address)
	{
		case 0x4000:
			ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
			return *soundlatch;
	}

	return 0;
}

static int DrvDoReset()
{
	DrvReset = 0;

	memset (AllRam, 0, RamEnd - AllRam);

	for (int i = 0; i < 3; i++) {
		ZetOpen(i);
		ZetReset();
		ZetClose();
	}

	enable_interrupt[0] = 0;
	enable_interrupt[1] = 0;
	disable_cpu[0] = 1;
	disable_cpu[1] = 1;

	protdata_last = 0;
	protdata = 0;

	watchdog = 0;

	return 0;
}

static int MemIndex()
{
	unsigned char *Next; Next = AllMem;

	DrvZ80ROM0		= Next; Next += 0x010000;
	DrvZ80ROM1		= Next; Next += 0x010000;
	DrvZ80ROM2		= Next; Next += 0x010000;

	DrvMCUROM		= Next; Next += 0x000800;

	DrvGfxROM0		= Next; Next += 0x010000;
	DrvGfxROM1		= Next; Next += 0x010000;
	DrvGfxROM2		= Next; Next += 0x008000;

	DrvColPROM		= Next; Next += 0x000b00; 

	AllRam			= Next;

	DrvFgVRAM		= Next; Next += 0x000800;
	DrvBgVRAM		= Next; Next += 0x000800;
	DrvShareRAM		= Next; Next += 0x001800;

	DrvZ80RAM2		= Next; Next += 0x000800;

	coinlockout		= Next; Next += 0x000001;
	soundlatch		= Next; Next += 0x000001;
	soundlatch2		= Next; Next += 0x000001;
	flipscreen		= Next; Next += 0x000001;

	gfxbank			= Next; Next += 0x000002;

	RamEnd			= Next;

	DrvColTable		= Next; Next += 0x000a00;
	Palette			= (unsigned int*)Next; Next += 0x0a00 * sizeof(int);
	DrvPalette		= (unsigned int*)Next; Next += 0x0a00 * sizeof(int);

	DrvTileOfst		= (unsigned short*)Next; Next += 0x0400 * sizeof(short);

	MemEnd			= Next;

	return 0;
}

static int DrvGfxDecode()
{
	int Plane0[1]  = { 0x00000 };
	int Plane1[4]  = { 0x00000, 0x20004, 0x20000, 0x00004 };
	int XOffs0[8]  = { 0x007, 0x006, 0x005, 0x004, 0x003, 0x002, 0x001, 0x000 };
	int XOffs1[16] = { 0x000, 0x001, 0x002, 0x003, 0x040, 0x041, 0x042, 0x043,
			   0x080, 0x081, 0x082, 0x083, 0x0c0, 0x0c1, 0x0c2, 0x0c3 };
	int YOffs[16]  = { 0x000, 0x008, 0x010, 0x018, 0x020, 0x028, 0x030, 0x038,
			   0x100, 0x108, 0x110, 0x118, 0x120, 0x128, 0x130, 0x138 };

	unsigned char *tmp = (unsigned char*)malloc(0x8000);
	if (tmp == NULL) {
		return 1;
	}

	memcpy (tmp, DrvGfxROM0, 0x2000);

	GfxDecode(0x0200, 1,  8,  8, Plane0, XOffs0, YOffs, 0x040, tmp, DrvGfxROM0);

	memcpy (tmp, DrvGfxROM1, 0x8000);

	GfxDecode(0x0100, 4, 16, 16, Plane1, XOffs1, YOffs, 0x200, tmp, DrvGfxROM1);

	memcpy (tmp, DrvGfxROM2, 0x8000);

	GfxDecode(0x0200, 4,  8,  8, Plane1, XOffs1, YOffs, 0x080, tmp, DrvGfxROM2);


	free (tmp);

	return 0;
}

static void DrvPaletteInit()
{
	for (int i = 0; i < 0x100; i++)
	{
		int r = DrvColPROM[i + 0x000] & 0x0f;
		int g = DrvColPROM[i + 0x100] & 0x0f;
		int b = DrvColPROM[i + 0x200] & 0x0f;

		r |= r << 4;
		g |= g << 4;
		b |= b << 4;

		DrvPalette[i] = (r << 16) | (g << 8) | b;
	}

	for (int i = 0; i < 0x200; i++) { // foreground
		DrvColTable[i] = (i >> 1) * (i & 1);
		Palette[i + 0x000] = DrvPalette[DrvColTable[i]];
	}

	for (int i = 0; i < 0x800; i++) { // everything else
		DrvColTable[i + 0x200] = BITSWAP08(DrvColPROM[i + 0x300],4,5,6,7,3,2,1,0);
		Palette[i + 0x200] = DrvPalette[DrvColTable[i + 0x200]];
		DrvColTable[i + 0x200] = (DrvColTable[i + 0x200] == 0xff) ? 0 : 1;
	}

	DrvRecalc = 1;
}

// Calculate offsets for tiles on screen
static void DrvCalculateOffsets()
{
	for (int offs = 0; offs < 36 * 28; offs++) {
		int col = (offs % 36) - 2;
		int row = (offs / 36) + 2;

		if (col & 0x20)
			DrvTileOfst[offs] = ((col & 0x1f) << 5) + row;
		else
			DrvTileOfst[offs] = (row << 5) + col;
	}
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
		if (BurnLoadRom(DrvZ80ROM0 + 0x00000,  0, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x02000,  1, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x04000,  2, 1)) return 1;

		if (BurnLoadRom(DrvZ80ROM1 + 0x00000,  3, 1)) return 1;

		if (BurnLoadRom(DrvZ80ROM2 + 0x00000,  4, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM0 + 0x00000,  5, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x00000,  6, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x02000,  7, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x04000,  8, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x06000,  9, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM2 + 0x00000, 10, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x04000, 11, 1)) return 1;

		if (BurnLoadRom(DrvColPROM + 0x00000, 12, 1)) return 1;
		if (BurnLoadRom(DrvColPROM + 0x00100, 13, 1)) return 1;
		if (BurnLoadRom(DrvColPROM + 0x00200, 14, 1)) return 1;
		if (BurnLoadRom(DrvColPROM + 0x00300, 15, 1)) return 1;

	//	if (BurnLoadRom(DrvMCUROM  + 0x00000, 16, 1)) return 1;

		DrvCalculateOffsets();
		DrvPaletteInit();
		DrvGfxDecode();
	}

	ZetInit(3);
	ZetOpen(0);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM0);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM0);
	ZetMapArea(0x8000, 0x87ff, 0, DrvFgVRAM);
	ZetMapArea(0x8000, 0x87ff, 1, DrvFgVRAM);
	ZetMapArea(0x8000, 0x87ff, 2, DrvFgVRAM);
	ZetMapArea(0x8800, 0x9fff, 0, DrvShareRAM);
	ZetMapArea(0x8800, 0x9fff, 1, DrvShareRAM);
	ZetMapArea(0x8800, 0x9fff, 2, DrvShareRAM);
	ZetMapArea(0xa000, 0xa7ff, 0, DrvBgVRAM);
	ZetMapArea(0xa000, 0xa7ff, 1, DrvBgVRAM);
	ZetMapArea(0xa000, 0xa7ff, 2, DrvBgVRAM);
	ZetSetWriteHandler(retofinv_main_write);
	ZetSetReadHandler(retofinv_main_read);
	ZetMemEnd();
	ZetClose();

	ZetOpen(1);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM1);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM1);
	ZetMapArea(0x8000, 0x87ff, 0, DrvFgVRAM);
	ZetMapArea(0x8000, 0x87ff, 1, DrvFgVRAM);
	ZetMapArea(0x8000, 0x87ff, 2, DrvFgVRAM);
	ZetMapArea(0x8800, 0x9fff, 0, DrvShareRAM);
	ZetMapArea(0x8800, 0x9fff, 1, DrvShareRAM);
	ZetMapArea(0x8800, 0x9fff, 2, DrvShareRAM);
	ZetMapArea(0xa000, 0xa7ff, 0, DrvBgVRAM);
	ZetMapArea(0xa000, 0xa7ff, 1, DrvBgVRAM);
	ZetMapArea(0xa000, 0xa7ff, 2, DrvBgVRAM);
	ZetSetWriteHandler(retofinv_main_write);
	ZetSetReadHandler(retofinv_main_read);
	ZetMemEnd();
	ZetClose();

	ZetOpen(2);
	ZetMapArea(0x0000, 0x1fff, 0, DrvZ80ROM2);
	ZetMapArea(0x0000, 0x1fff, 2, DrvZ80ROM2);
	ZetMapArea(0x2000, 0x27ff, 0, DrvZ80RAM2);
	ZetMapArea(0x2000, 0x27ff, 1, DrvZ80RAM2);
	ZetMapArea(0x2000, 0x27ff, 2, DrvZ80RAM2);
	ZetMapArea(0xe000, 0xffff, 0, DrvZ80ROM2 + 0xe000);
	ZetMapArea(0xe000, 0xffff, 2, DrvZ80ROM2 + 0xe000);
	ZetSetWriteHandler(retofinv_sound_write);
	ZetSetReadHandler(retofinv_sound_read);
	ZetClose();

	SN76489Init(0, 18432000 / 6, 0);
	SN76489Init(1, 18432000 / 6, 1);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	GenericTilesExit();

	ZetExit();

	SN76496Exit();

	free (AllMem);
	AllMem = NULL;

	return 0;
}

static void draw_background_layer()
{
	for (int offs = 0; offs < 36 * 28; offs++)
	{
		int sx = (offs % 36) << 3;
		int sy = (offs / 36) << 3;


		int ofst = DrvTileOfst[offs];

		int code  = DrvBgVRAM[ofst] | (gfxbank[1] << 8);
		int color = (DrvBgVRAM[ofst + 0x400] & 0x3f) + 0x60;

		if (*flipscreen) {
			Render8x8Tile_FlipXY_Clip(pTransDraw, code, 280 - sx, 216 - sy, color, 4, 0, DrvGfxROM2);
		} else {
			Render8x8Tile_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
		}
	}
}

static void draw_foreground_layer()
{
	int flip = *flipscreen ? 0x3f : 0;

	for (int offs = 0; offs < 36 * 28; offs++)
	{
		int sx = (offs % 36) << 3;
		int sy = (offs / 36) << 3;

		if (*flipscreen) {
			sx = 280 - sx;
			sy = 216 - sy;
		}

		int ofst = DrvTileOfst[offs];

		int code  = DrvFgVRAM[ofst + 0x000] | (gfxbank[0] << 8);
		int color = DrvFgVRAM[ofst + 0x400];

		{
			color <<= 1;
			unsigned char *src = DrvGfxROM0 + code * 0x40;
			unsigned short *dst = pTransDraw + (sy * nScreenWidth) + sx;

			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					int pxl = src[((y << 3) | x) ^ flip] | color;

					if (DrvColTable[pxl]) {
						dst[x] = pxl;
					}
				}
				dst += nScreenWidth;
			}
		}
	}
}

static inline void draw_masked_sprite(int code, int color, int sx, int sy, int fx, int fy)
{
	unsigned short *dst;
	unsigned char *src = DrvGfxROM1 + code * 0x100;

	if (fx) fx  = 0x0f;
	if (fy) fx |= 0xf0;

	for (int y = 0; y < 16; y++, sy++) {
		if (sy < 0 || sy >= nScreenHeight) continue;

		dst = pTransDraw + sy * nScreenWidth;

		for (int x = 0; x < 16; x++, sx++) {
			if (sx < 0 || sx >= nScreenWidth) continue;

			int pxl = src[((y << 4) | x) ^ fx] | color;

			if (DrvColTable[pxl]) {
				dst[sx] = pxl;
			}
		}

		sx -= 16;
	}
}

static void draw_sprites()
{
	static const int gfx_offs[2][2] =
	{
		{ 0, 1 },
		{ 2, 3 }
	};

	unsigned char *ram0 = DrvShareRAM + 0x780;
	unsigned char *ram1 = DrvShareRAM + 0xf80;
	unsigned char *ram2 = DrvShareRAM + 0x1780;

	for (int offs = 0; offs < 0x80; offs += 2)
	{
		int attr  =  ram2[offs];
		int sprite=  ram0[offs+0];
		int color = (ram0[offs+1] & 0x3f) + 0x20;
		int sx    = ((ram1[offs+1] << 1) + ((ram2[offs+1] & 0x80) >> 7)) - 39;
		int sy    = 256 - ((ram1[offs] << 1) + ((attr & 0x80) >> 7)) + 1;
		int flipx = (attr & 0x01);
		int flipy = (attr & 0x02) >> 1;
		int sizey = (attr & 0x04) >> 2;
		int sizex = (attr & 0x08) >> 3;

		sprite &= ~sizex;
		sprite &= ~(sizey << 1);

		if (*flipscreen)
		{
			flipx ^= 1;
			flipy ^= 1;
		}

		sy -= sizey << 4;
		sy = (sy & 0xff) - 32;

		for (int y = 0; y <= sizey; y++)
		{
			for (int x = 0; x <= sizex; x++)
			{
				int code = sprite + gfx_offs[y ^ (sizey * flipy)][x ^ (sizex * flipx)];				
				draw_masked_sprite(code, color << 4, sx + 16*x, sy + 16*y, flipx, flipy);
			}
		}
	}
}

static int DrvDraw()
{
	if (DrvRecalc) {
		for (int i = 0; i < 0xa00; i++) {
			int d = Palette[i];
			DrvPalette[i] = BurnHighCol(d >> 16, (d >> 8) & 0xff, d & 0xff, 0);
		}
		DrvRecalc = 0;
	}

	draw_background_layer();
	draw_sprites();
	draw_foreground_layer();

	BurnTransferCopy(DrvPalette);

	return 0;
}

static int DrvFrame()
{
	watchdog++;
	if (DrvReset || watchdog > 59) {
		DrvDoReset();
	}

	ZetNewFrame();

	{
		DrvInputs[0] = 0xff;
		DrvInputs[1] = 0xff;
		DrvInputs[2] = 0xcf;
		for (int i = 0; i < 8; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
			DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
		}
	}

	int nInterleave = 100;
	int nCyclesTotal[3] = { 3072000 / 60, 3072000 / 60, 3072000 / 60 };
	int nCyclesDone[3] = { 0, 0, 0 };

	for (int i = 0; i < nInterleave; i++)
	{
		int nCycleSegment = (nCyclesTotal[0] / nInterleave);

		ZetOpen(0);
		nCyclesDone[0] += ZetRun(nCycleSegment);
		if (i == (nInterleave - 1) && enable_interrupt[0]) ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
		ZetClose();

		nCycleSegment *= (i + 1);

		if (disable_cpu[0]) {
			ZetOpen(1);
			nCyclesDone[1] += ZetRun(nCycleSegment - ZetTotalCycles());
			if (i == (nInterleave - 1) && enable_interrupt[1]) ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
			ZetClose();
		}

		if (disable_cpu[1]) {
			ZetOpen(2);
			nCyclesDone[1] += ZetRun(nCycleSegment - ZetTotalCycles());
			if (i == (nInterleave - 1) || i == (nInterleave / 2)-1) ZetNmi();
			ZetClose();
		}
	}
	
	SN76496Update(0, pBurnSoundOut, nBurnSoundLen);
	SN76496Update(1, pBurnSoundOut, nBurnSoundLen);

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static int DrvScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029707;
	}

	if (nAction & ACB_VOLATILE) {		
		memset(&ba, 0, sizeof(ba));

		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd - AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);

		ZetScan(nAction);
		SN76496Scan(nAction, pnMin);

		SCAN_VAR(enable_interrupt[0]);
		SCAN_VAR(enable_interrupt[1]);
		SCAN_VAR(disable_cpu[0]);
		SCAN_VAR(disable_cpu[1]);
		SCAN_VAR(protdata_last);
		SCAN_VAR(protdata);
	}

	return 0;
}


// Return of the Invaders

static struct BurnRomInfo retofinvRomDesc[] = {
	{ "a37-03.70",	0x2000, 0xeae7459d, 1 | BRF_PRG | BRF_ESS }, //  0 Z80 #0 Code
	{ "a37-02.71",	0x2000, 0x72895e37, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "a37-01.72",	0x2000, 0x505dd20b, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "a37-04.62",	0x2000, 0xd2899cc1, 2 | BRF_PRG | BRF_ESS }, //  3 Z80 #1 Code

	{ "a37-05.17",	0x2000, 0x9025abea, 3 | BRF_PRG | BRF_ESS }, //  4 Z80 #2 Code

	{ "a37-16.61",	0x2000, 0x4e3f501c, 4 | BRF_GRA },           //  5 Characters

	{ "a37-10.8",	0x2000, 0x6afdeec8, 5 | BRF_GRA },           //  6 Sprites
	{ "a37-11.9",	0x2000, 0xd3dc9da3, 5 | BRF_GRA },           //  7
	{ "a37-12.10",	0x2000, 0xd10b2eed, 5 | BRF_GRA },           //  8
	{ "a37-13.11",	0x2000, 0x00ca6b3d, 5 | BRF_GRA },           //  9

	{ "a37-14.55",	0x2000, 0xef7f8651, 6 | BRF_GRA },           // 10 Tiles
	{ "a37-15.56",	0x2000, 0x03b40905, 6 | BRF_GRA },           // 11

	{ "a37-06.13",	0x0100, 0xe9643b8b, 7 | BRF_GRA },           // 12 Color Proms
	{ "a37-07.4",	0x0100, 0xe8f34e11, 7 | BRF_GRA },           // 13
	{ "a37-08.3",	0x0100, 0x50030af0, 7 | BRF_GRA },           // 14
	{ "82s191n",	0x0800, 0x93c891e3, 7 | BRF_GRA },           // 15

	{ "a37-09.37",	0x0800, 0x79bd6ded, 8 | BRF_PRG | BRF_OPT }, // 16 68705 Code (not used)
};

STD_ROM_PICK(retofinv)
STD_ROM_FN(retofinv)

struct BurnDriver BurnDrvRetofinv = {
	"retofinv", NULL, NULL, "1985",
	"Return of the Invaders\0", NULL, "Taito Corporation", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_MISC_PRE90S, GBF_SHOOT, 0,
	NULL, retofinvRomInfo, retofinvRomName, RetofinvInputInfo, RetofinvDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc, 0xa00,
	224, 288, 3, 4
};


// Return of the Invaders (bootleg set 1)

static struct BurnRomInfo retofin1RomDesc[] = {
	{ "roi.02",	0x2000, 0xd98fd462, 1 | BRF_PRG | BRF_ESS }, //  0 Z80 #0 Code
	{ "roi.01b",	0x2000, 0x3379f930, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "roi.01",	0x2000, 0x57679062, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "a37-04.62",	0x2000, 0xd2899cc1, 2 | BRF_PRG | BRF_ESS }, //  3 Z80 #1 Code

	{ "a37-05.17",	0x2000, 0x9025abea, 3 | BRF_PRG | BRF_ESS }, //  4 Z80 #2 Code

	{ "a37-16.61",	0x2000, 0x4e3f501c, 4 | BRF_GRA },           //  5 Characters

	{ "a37-10.8",	0x2000, 0x6afdeec8, 5 | BRF_GRA },           //  6 Sprites
	{ "a37-11.9",	0x2000, 0xd3dc9da3, 5 | BRF_GRA },           //  7
	{ "a37-12.10",	0x2000, 0xd10b2eed, 5 | BRF_GRA },           //  8
	{ "a37-13.11",	0x2000, 0x00ca6b3d, 5 | BRF_GRA },           //  9

	{ "a37-14.55",	0x2000, 0xef7f8651, 6 | BRF_GRA },           // 10 Tiles
	{ "a37-15.56",	0x2000, 0x03b40905, 6 | BRF_GRA },           // 11

	{ "a37-06.13",	0x0100, 0xe9643b8b, 7 | BRF_GRA },           // 12 Color Proms
	{ "a37-07.4",	0x0100, 0xe8f34e11, 7 | BRF_GRA },           // 13
	{ "a37-08.3",	0x0100, 0x50030af0, 7 | BRF_GRA },           // 14
	{ "82s191n",	0x0800, 0x93c891e3, 7 | BRF_GRA },           // 15
};

STD_ROM_PICK(retofin1)
STD_ROM_FN(retofin1)

struct BurnDriver BurnDrvRetofin1 = {
	"retofinv1", "retofinv", NULL, "1985",
	"Return of the Invaders (bootleg set 1)\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_MISC_PRE90S, GBF_SHOOT, 0,
	NULL, retofin1RomInfo, retofin1RomName, RetofinvInputInfo, RetofinvDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc, 0xa00,
	224, 288, 3, 4
};


// Return of the Invaders (bootleg set 2)

static struct BurnRomInfo retofin2RomDesc[] = {
	{ "ri-c.1e",	0x2000, 0xe3c31260, 1 | BRF_PRG | BRF_ESS }, //  0 Z80 #0 Code
	{ "roi.01b",	0x2000, 0x3379f930, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "ri-a.1c",	0x2000, 0x3ae7c530, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "a37-04.62",	0x2000, 0xd2899cc1, 2 | BRF_PRG | BRF_ESS }, //  3 Z80 #1 Code

	{ "a37-05.17",	0x2000, 0x9025abea, 3 | BRF_PRG | BRF_ESS }, //  4 Z80 #2 Code

	{ "a37-16.61",	0x2000, 0x4e3f501c, 4 | BRF_GRA },           //  5 Characters

	{ "a37-10.8",	0x2000, 0x6afdeec8, 5 | BRF_GRA },           //  6 Sprites
	{ "a37-11.9",	0x2000, 0xd3dc9da3, 5 | BRF_GRA },           //  7
	{ "a37-12.10",	0x2000, 0xd10b2eed, 5 | BRF_GRA },           //  8
	{ "a37-13.11",	0x2000, 0x00ca6b3d, 5 | BRF_GRA },           //  9

	{ "a37-14.55",	0x2000, 0xef7f8651, 6 | BRF_GRA },           // 10 Tiles
	{ "a37-15.56",	0x2000, 0x03b40905, 6 | BRF_GRA },           // 11

	{ "a37-06.13",	0x0100, 0xe9643b8b, 7 | BRF_GRA },           // 12 Color Proms
	{ "a37-07.4",	0x0100, 0xe8f34e11, 7 | BRF_GRA },           // 13
	{ "a37-08.3",	0x0100, 0x50030af0, 7 | BRF_GRA },           // 14
	{ "82s191n",	0x0800, 0x93c891e3, 7 | BRF_GRA },           // 15
};

STD_ROM_PICK(retofin2)
STD_ROM_FN(retofin2)

struct BurnDriver BurnDrvRetofin2 = {
	"retofinv2", "retofinv", NULL, "1985",
	"Return of the Invaders (bootleg set 2)\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_MISC_PRE90S, GBF_SHOOT, 0,
	NULL, retofin2RomInfo, retofin2RomName, RetofinvInputInfo, RetofinvDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc, 0xa00,
	224, 288, 3, 4
};
