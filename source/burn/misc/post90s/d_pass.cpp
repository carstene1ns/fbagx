// FB Alpha Pass driver module
// Based on MAME driver by David Haywood and Stephh

#include "tiles_generic.h"
#include "burn_ym2203.h"
#include "msm6295.h"

static unsigned char *AllMem;
static unsigned char *MemEnd;
static unsigned char *AllRam;
static unsigned char *RamEnd;
static unsigned char *Drv68KROM;
static unsigned char *Drv68KRAM;
static unsigned char *DrvZ80ROM;
static unsigned char *DrvZ80RAM;
static unsigned char *DrvSndROM;
static unsigned char *DrvGfxROM0;
static unsigned char *DrvGfxROM1;
static unsigned char *DrvPalRAM;
static unsigned char *DrvBgVRAM;
static unsigned char *DrvFgVRAM;
static unsigned int  *DrvPalette;

static unsigned char DrvRecalc;

static unsigned char DrvJoy1[16];
static unsigned char DrvDips[2];
static unsigned char DrvReset;
static unsigned short DrvInputs[2];

static unsigned char *soundlatch;

static struct BurnInputInfo PassInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 start"	},
	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 4,	"p1 up"		},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 5,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 6,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy1 + 7,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy1 + 3,	"p1 fire 2"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy1 + 8,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy1 + 9,	"p2 start"	},
	{"P2 Up",		BIT_DIGITAL,	DrvJoy1 + 12,	"p2 up"		},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy1 + 13,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy1 + 14,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy1 + 15,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy1 + 10,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy1 + 11,	"p2 fire 2"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"		},
	{"Dip A",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",		BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
};

STDINPUTINFO(Pass)

static struct BurnDIPInfo PassDIPList[]=
{
	{0x11, 0xff, 0xff, 0xff, NULL			},
	{0x12, 0xff, 0xff, 0xe7, NULL			},

	{0   , 0xfe, 0   ,    4, "Lives"		},
	{0x12, 0x01, 0x03, 0x00, "2"			},
	{0x12, 0x01, 0x03, 0x03, "3"			},
	{0x12, 0x01, 0x03, 0x01, "4"			},
	{0x12, 0x01, 0x03, 0x02, "5"			},

	{0   , 0xfe, 0   ,    4, "Difficulty"		},
	{0x12, 0x01, 0x18, 0x00, "Easy"			},
	{0x12, 0x01, 0x18, 0x18, "Normal"		},
	{0x12, 0x01, 0x18, 0x08, "Hard"			},
	{0x12, 0x01, 0x18, 0x10, "Hardest"		},

	{0   , 0xfe, 0   ,    7, "Coinage"		},
	{0x12, 0x01, 0xe0, 0x80, "4 Coins 1 Credits "	},
	{0x12, 0x01, 0xe0, 0x40, "3 Coins 1 Credits "	},
	{0x12, 0x01, 0xe0, 0xc0, "2 Coins 1 Credits "	},
	{0x12, 0x01, 0xe0, 0xe0, "1 Coin  1 Credits"	},
	{0x12, 0x01, 0xe0, 0x60, "1 Coin  2 Credits"	},
	{0x12, 0x01, 0xe0, 0xa0, "1 Coin  3 Credits"	},
	{0x12, 0x01, 0xe0, 0x20, "1 Coin  4 Credits"	},
};

STDDIPINFO(Pass)

void __fastcall pass_write_word(unsigned int address, unsigned short data)
{
	switch (address)
	{
		case 0x230000:
			*soundlatch = data & 0xff;
		return;
	}
}

void __fastcall pass_write_byte(unsigned int /*address*/, unsigned char /*data*/)
{

}

unsigned short __fastcall pass_read_word(unsigned int address)
{
	switch (address)
	{
		case 0x230100:
			return DrvInputs[1];

		case 0x230200:
			return DrvInputs[0];
	}

	return 0;
}

unsigned char __fastcall pass_read_byte(unsigned int /*address*/)
{
	return 0;
}

void __fastcall pass_sound_write_port(unsigned short port, unsigned char data)
{
	switch (port & 0xff)
	{
		case 0x70:
			BurnYM2203Write(0, 0, data);
		return;

		case 0x71:
			BurnYM2203Write(0, 1, data);
		return;

		case 0x80:
			MSM6295Command(0, data);
		return;

		case 0xc0:
			*soundlatch = 0;
		return;
	}
}

unsigned char __fastcall pass_sound_read_port(unsigned short port)
{
	switch (port & 0xff)
	{
		case 0x00:
			return *soundlatch;

		case 0x70:
			return BurnYM2203Read(0, 0);

		case 0x71:
			return BurnYM2203Read(0, 1);
	}

	return 0;
}

static int DrvSynchroniseStream(int nSoundRate)
{
	return (long long)ZetTotalCycles() * nSoundRate / 3579545;
}

static double DrvGetTime()
{
	return (double)ZetTotalCycles() / 3579545.0;
}

static int DrvDoReset()
{
	DrvReset = 0;

	memset (AllRam, 0, RamEnd - AllRam);

	SekOpen(0);
	SekReset();
	SekClose();

	ZetOpen(0);
	ZetReset();
	ZetClose();

	BurnYM2203Reset();
	MSM6295Reset(0);

	return 0;
}

static int MemIndex()
{
	unsigned char *Next; Next = AllMem;

	Drv68KROM	= Next; Next += 0x040000;
	DrvZ80ROM	= Next; Next += 0x010000;

	DrvGfxROM0	= Next; Next += 0x040000;
	DrvGfxROM1	= Next; Next += 0x080000;

	MSM6295ROM	= Next;
	DrvSndROM	= Next; Next += 0x020000;

	DrvPalette	= (unsigned int*)Next; Next += 0x0200 * sizeof(int);

	AllRam		= Next;

	Drv68KRAM	= Next; Next += 0x004000;
	DrvPalRAM	= Next; Next += 0x000400;
	DrvBgVRAM	= Next; Next += 0x001000;
	DrvFgVRAM	= Next; Next += 0x004000;

	DrvZ80RAM	= Next; Next += 0x000800;

	soundlatch	= Next; Next += 0x000001;

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
		if (BurnLoadRom(Drv68KROM + 0x0000000,	 0, 2)) return 1;
		if (BurnLoadRom(Drv68KROM + 0x0000001,	 1, 2)) return 1;

		if (BurnLoadRom(DrvZ80ROM,		 2, 1)) return 1;

		if (BurnLoadRom(DrvSndROM,		 3, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM0 + 0x000000,	 4, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM0 + 0x000001,	 5, 2)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x000000,	 6, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x000001,	 7, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x040000,	 8, 2)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x040001,	 9, 2)) return 1;
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x03ffff, SM_ROM);
	SekMapMemory(Drv68KRAM,		0x080000, 0x083fff, SM_RAM);
	SekMapMemory(DrvBgVRAM,		0x200000, 0x200fff, SM_RAM);
	SekMapMemory(DrvFgVRAM,		0x210000, 0x213fff, SM_RAM);
	SekMapMemory(DrvPalRAM,		0x220000, 0x2203ff, SM_RAM);
	SekSetWriteWordHandler(0,	pass_write_word);
	SekSetWriteByteHandler(0,	pass_write_byte);
	SekSetReadWordHandler(0,	pass_read_word);
	SekSetReadByteHandler(0,	pass_read_byte);
	SekClose();

	ZetInit(1);
	ZetOpen(0);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM);
	ZetMapArea(0xf800, 0xffff, 0, DrvZ80RAM);
	ZetMapArea(0xf800, 0xffff, 1, DrvZ80RAM);
	ZetMapArea(0xf800, 0xffff, 2, DrvZ80RAM);
	ZetSetOutHandler(pass_sound_write_port);
	ZetSetInHandler(pass_sound_read_port);
	ZetMemEnd();
	ZetClose();

	BurnYM2203Init(1, 3579545, NULL, DrvSynchroniseStream, DrvGetTime, 0);

	MSM6295Init(0, 792000 / 132, 100.0, 1);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	GenericTilesExit();

	BurnYM2203Exit();
	MSM6295Exit(0);
	SekExit();
	ZetExit();

	free (AllMem);
	AllMem = NULL;

	MSM6295ROM = NULL;

	return 0;
}

static void draw_foreground_layer()
{
	unsigned short *vram = (unsigned short*)DrvFgVRAM;

	for (int offs = 0; offs < 128 * 64; offs++)
	{
		int sx = ((offs & 0x7f) << 2) - 64;
		int sy = ((offs >> 7) << 2) - 16;

		if (sx >= nScreenWidth || sx < -3 || sy >= nScreenHeight || sy < -3) continue;

		int attr  = vram[offs];
		int code  = attr & 0x3fff;
		int flipy = (attr & 0x8000) >> 15;
		int flipx = (attr & 0x4000) >> 14;

		if (code < 0x10) continue;

		int flip = (flipy * 0x0c) | (flipx * 3);
		unsigned char *src = DrvGfxROM0 + (code << 4);

		for (int y = 0; y < 4; y++, sy++)
		{
			for (int x = 0; x < 4; x++)
			{
				int pxl = src[((y << 2) | x) ^ flip];
				if (pxl == 0xff) continue;

				pTransDraw[(sy * nScreenWidth) + (sx + x)] = pxl | 0x100;
			}
		}
	}
}

static void draw_background_layer()
{
	unsigned short *vram = (unsigned short*)DrvBgVRAM;

	for (int offs = 0; offs < 64 * 32; offs++)
	{
		int sx = ((offs & 0x3f) << 3) - 64;
		int sy = ((offs >> 6) << 3) - 16;

		if (sx >= nScreenWidth || sx < -7 || sy >= nScreenHeight || sy < -7) continue;

		int attr  = vram[offs];
		int code  = attr & 0x1fff;
		int flipy = attr & 0x8000;
		int flipx = attr & 0x4000;

		if (flipy) {
			if (flipx) {
				Render8x8Tile_FlipXY(pTransDraw, code, sx, sy, 0, 8, 0, DrvGfxROM1);
			} else {
				Render8x8Tile_FlipY(pTransDraw, code, sx, sy, 0, 8, 0, DrvGfxROM1);
			}
		} else {
			if (flipx) {
				Render8x8Tile_FlipX(pTransDraw, code, sx, sy, 0, 8, 0, DrvGfxROM1);
			} else {
				Render8x8Tile(pTransDraw, code, sx, sy, 0, 8, 0, DrvGfxROM1);
			}
		}
	}
}

static int DrvDraw()
{
	if (DrvRecalc) {
		unsigned char r,g,b;
		unsigned short *pal = (unsigned short*)DrvPalRAM;
		for (int i = 0; i < 0x200; i++) {
			int d = pal[i];

			r = ((d >> 10) & 0x1f);
			g = ((d >>  5) & 0x1f);
			b = ((d >>  0) & 0x1f);

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			DrvPalette[i] = BurnHighCol(r, g, b, 0);
		}
	}

	draw_background_layer();
	draw_foreground_layer();

	BurnTransferCopy(DrvPalette);

	return 0;
}


static int DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	SekNewFrame();
	ZetNewFrame();

	{
		DrvInputs[0] = ~0;
		DrvInputs[1] = DrvDips[0] | (DrvDips[1] << 8);

		for (int i = 0; i < 16; i++)
		{
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
		}
	}

	int nSegment;
	int nInterleave = 10;
	int nTotalCycles[2] = { 7159090 / 60, 3579545 / 60 };
	int nCyclesDone[2] = { 0, 0 };

	SekOpen(0);
	ZetOpen(0);

	for (int i = 0; i < nInterleave; i++)
	{
		nSegment = (nTotalCycles[0] - nCyclesDone[0]) / (nInterleave - i);

		nCyclesDone[0] += SekRun(nSegment);

		nSegment = (nTotalCycles[1] - nCyclesDone[1]) / (nInterleave - i);

		nCyclesDone[1] += ZetRun(nSegment);
	}

	if (pBurnSoundOut) {
		BurnYM2203Update(pBurnSoundOut, nBurnSoundLen);
		MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
	}

	SekSetIRQLine(1, SEK_IRQSTATUS_AUTO);

	ZetClose();
	SekClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {
		*pnMin = 0x029703;
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
		ZetScan(nAction);

		BurnYM2203Scan(nAction, pnMin);
		MSM6295Scan(0, nAction);
	}

	return 0;
}


// Pass

static struct BurnRomInfo passRomDesc[] = {
	{ "33",		0x20000, 0x0c5f18f6, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "34",		0x20000, 0x7b54573d, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "23",		0x10000, 0xb9a0ccde, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "31",		0x20000, 0xc7315bbd, 3 | BRF_SND },           //  3 Samples

	{ "35",		0x20000, 0x2ab33f07, 4 | BRF_GRA },           //  4 Foreground Tiles
	{ "36",		0x20000, 0x6677709d, 4 | BRF_GRA },           //  5

	{ "38",		0x20000, 0x7f11b81a, 5 | BRF_GRA },           //  6 Background Tiles
	{ "40",		0x20000, 0x80e0a71d, 5 | BRF_GRA },           //  7
	{ "37",		0x20000, 0x296499e7, 5 | BRF_GRA },           //  8
	{ "39",		0x20000, 0x35c0ad5c, 5 | BRF_GRA },           //  9
};

STD_ROM_PICK(pass)
STD_ROM_FN(pass)

struct BurnDriver BurnDrvPass = {
	"pass", NULL, NULL, "1992",
	"Pass\0", NULL, "Oksan", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_MAZE, 0,
	NULL, passRomInfo, passRomName, PassInputInfo, PassDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc, 0x200,
	320, 224, 4, 3
};
