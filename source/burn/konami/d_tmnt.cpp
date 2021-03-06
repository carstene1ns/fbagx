#include "tiles_generic.h"
#include "konamiic.h"
#include "burn_ym2151.h"
#include "upd7759.h"
#include "k007232.h"
#include "k053260.h"
#include "eeprom.h"

static unsigned char DrvInputPort0[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort3[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort4[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort5[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvDip[3]        = {0, 0, 0};
static unsigned char DrvInput[6]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char DrvReset         = 0;

static unsigned char *Mem                 = NULL;
static unsigned char *MemEnd              = NULL;
static unsigned char *RamStart            = NULL;
static unsigned char *RamEnd              = NULL;
static unsigned char *Drv68KRom           = NULL;
static unsigned char *Drv68KRam           = NULL;
static unsigned char *DrvZ80Rom           = NULL;
static unsigned char *DrvZ80Ram           = NULL;
static unsigned char *DrvSoundRom         = NULL;
static unsigned char *DrvUPD7759CRom      = NULL;
static unsigned char *DrvTileRom          = NULL;
static unsigned char *DrvSpriteRom        = NULL;
static unsigned char *DrvSpriteRam        = NULL;
static unsigned char *DrvPaletteRam       = NULL;
static unsigned char *DrvNvRam            = NULL;
static unsigned char *DrvTiles            = NULL;
static unsigned char *DrvSprites          = NULL;
static unsigned char *DrvTempRom          = NULL;
static unsigned int  *Palette;
static unsigned int  *DrvPalette          = NULL;
static INT16         *DrvTitleSample      = NULL;

static int nCyclesDone[2], nCyclesTotal[2];
static int nCyclesSegment;

static unsigned char bIrqEnable;
static int LayerColourBase[3];
static int SpriteColourBase;
static int LayerPri[3];
static unsigned char DrvSoundLatch;
static int PriorityFlag;

static int dim_c, dim_v;

static int DrvNvRamBank;
static int CuebrickSndIrqFire;
static int BlswhstlTileRomBank;

static int TitleSoundLatch;
static int PlayTitleSample;
double TitleSamplePos = 0;

static unsigned char DrvVBlank;

static int InitEEPROMCount;

static const eeprom_interface BlswhstlEEPROMInterface =
{
	7,
	8,
	"011000",
	"011100",
	0,
	"0100000000000",
	"0100110000000",
	0,
	0
};

static const eeprom_interface thndrx2_eeprom_interface =
{
	7,
	8,
	"011000",
	"010100",
	0,
	"0100000000000",
	"0100110000000",
	0,
	0
};

static struct BurnInputInfo TmntInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 coin"   },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 coin"   },
	{"Coin 3"            , BIT_DIGITAL  , DrvInputPort0 + 2, "p3 coin"   },
	{"Coin 4"            , BIT_DIGITAL  , DrvInputPort0 + 3, "p4 coin"   },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort1 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p1 fire 2" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort2 + 0, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort2 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 fire 2" },
	
	{"P3 Up"             , BIT_DIGITAL  , DrvInputPort3 + 2, "p3 up"     },
	{"P3 Down"           , BIT_DIGITAL  , DrvInputPort3 + 3, "p3 down"   },
	{"P3 Left"           , BIT_DIGITAL  , DrvInputPort3 + 0, "p3 left"   },
	{"P3 Right"          , BIT_DIGITAL  , DrvInputPort3 + 1, "p3 right"  },
	{"P3 Fire 1"         , BIT_DIGITAL  , DrvInputPort3 + 4, "p3 fire 1" },
	{"P3 Fire 2"         , BIT_DIGITAL  , DrvInputPort3 + 5, "p3 fire 2" },
	
	{"P4 Up"             , BIT_DIGITAL  , DrvInputPort4 + 2, "p4 up"     },
	{"P4 Down"           , BIT_DIGITAL  , DrvInputPort4 + 3, "p4 down"   },
	{"P4 Left"           , BIT_DIGITAL  , DrvInputPort4 + 0, "p4 left"   },
	{"P4 Right"          , BIT_DIGITAL  , DrvInputPort4 + 1, "p4 right"  },
	{"P4 Fire 1"         , BIT_DIGITAL  , DrvInputPort4 + 4, "p4 fire 1" },
	{"P4 Fire 2"         , BIT_DIGITAL  , DrvInputPort4 + 5, "p4 fire 2" },

	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service 1"         , BIT_DIGITAL  , DrvInputPort0 + 4, "service"   },
	{"Service 2"         , BIT_DIGITAL  , DrvInputPort0 + 5, "service2"  },
	{"Service 3"         , BIT_DIGITAL  , DrvInputPort0 + 6, "service3"  },
	{"Service 4"         , BIT_DIGITAL  , DrvInputPort0 + 7, "service4"  },
	{"Dip 1"             , BIT_DIPSWITCH, DrvDip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, DrvDip + 1       , "dip"       },
	{"Dip 3"             , BIT_DIPSWITCH, DrvDip + 2       , "dip"       },
};

STDINPUTINFO(Tmnt)

static struct BurnInputInfo Tmnt2pInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort1 + 7, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort2 + 7, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort1 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p1 fire 2" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort2 + 0, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort2 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 fire 2" },

	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service 1"         , BIT_DIGITAL  , DrvInputPort0 + 4, "service"   },
	{"Service 2"         , BIT_DIGITAL  , DrvInputPort0 + 5, "service2"  },
	{"Dip 1"             , BIT_DIPSWITCH, DrvDip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, DrvDip + 1       , "dip"       },
	{"Dip 3"             , BIT_DIPSWITCH, DrvDip + 2       , "dip"       },
};

STDINPUTINFO(Tmnt2p)

static struct BurnInputInfo MiaInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort0 + 3, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort0 + 4, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort1 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p1 fire 2" },
	{"P1 Fire 3"         , BIT_DIGITAL  , DrvInputPort1 + 6, "p1 fire 3" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort2 + 0, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort2 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 fire 2" },
	{"P2 Fire 3"         , BIT_DIGITAL  , DrvInputPort2 + 6, "p2 fire 3" },
	
	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , DrvInputPort0 + 6, "service"   },
	{"Dip 1"             , BIT_DIPSWITCH, DrvDip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, DrvDip + 1       , "dip"       },
	{"Dip 3"             , BIT_DIPSWITCH, DrvDip + 2       , "dip"       },
};

STDINPUTINFO(Mia)

static struct BurnInputInfo BlswhstlInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort0 + 4, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort0 + 5, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort1 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p1 fire 2" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort2 + 0, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort2 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 fire 2" },
	
	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , DrvInputPort0 + 2, "service"   },
	{"Diagnostics"       , BIT_DIGITAL  , DrvInputPort0 + 3, "diag"      },
};

STDINPUTINFO(Blswhstl)

static struct BurnInputInfo Ssriders4pInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 coin"   },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 coin"   },
	{"Coin 3"            , BIT_DIGITAL  , DrvInputPort0 + 2, "p3 coin"   },
	{"Coin 4"            , BIT_DIGITAL  , DrvInputPort0 + 3, "p4 coin"   },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort1 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p1 fire 2" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort2 + 0, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort2 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 fire 2" },
	
	{"P3 Up"             , BIT_DIGITAL  , DrvInputPort3 + 2, "p3 up"     },
	{"P3 Down"           , BIT_DIGITAL  , DrvInputPort3 + 3, "p3 down"   },
	{"P3 Left"           , BIT_DIGITAL  , DrvInputPort3 + 0, "p3 left"   },
	{"P3 Right"          , BIT_DIGITAL  , DrvInputPort3 + 1, "p3 right"  },
	{"P3 Fire 1"         , BIT_DIGITAL  , DrvInputPort3 + 4, "p3 fire 1" },
	{"P3 Fire 2"         , BIT_DIGITAL  , DrvInputPort3 + 5, "p3 fire 2" },
	
	{"P4 Up"             , BIT_DIGITAL  , DrvInputPort4 + 2, "p4 up"     },
	{"P4 Down"           , BIT_DIGITAL  , DrvInputPort4 + 3, "p4 down"   },
	{"P4 Left"           , BIT_DIGITAL  , DrvInputPort4 + 0, "p4 left"   },
	{"P4 Right"          , BIT_DIGITAL  , DrvInputPort4 + 1, "p4 right"  },
	{"P4 Fire 1"         , BIT_DIGITAL  , DrvInputPort4 + 4, "p4 fire 1" },
	{"P4 Fire 2"         , BIT_DIGITAL  , DrvInputPort4 + 5, "p4 fire 2" },
	
	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service 1"         , BIT_DIGITAL  , DrvInputPort0 + 4, "service"   },
	{"Service 2"         , BIT_DIGITAL  , DrvInputPort0 + 5, "service 2" },
	{"Service 3"         , BIT_DIGITAL  , DrvInputPort0 + 6, "service 3" },
	{"Service 4"         , BIT_DIGITAL  , DrvInputPort0 + 7, "service 4" },
	{"Diagnostics"       , BIT_DIGITAL  , DrvInputPort5 + 7, "diag"      },
};

STDINPUTINFO(Ssriders4p)

static struct BurnInputInfo SsridersInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort1 + 7, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort2 + 7, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort1 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p1 fire 2" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort2 + 0, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort2 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 fire 2" },
	
	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service 1"         , BIT_DIGITAL  , DrvInputPort0 + 4, "service"   },
	{"Service 2"         , BIT_DIGITAL  , DrvInputPort0 + 5, "service 2" },
	{"Diagnostics"       , BIT_DIGITAL  , DrvInputPort5 + 7, "diag"      },
};

STDINPUTINFO(Ssriders)

static struct BurnInputInfo Ssriders4psInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort1 + 7, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort2 + 7, "p2 start"  },
	{"Coin 3"            , BIT_DIGITAL  , DrvInputPort0 + 2, "p3 coin"   },
	{"Start 3"           , BIT_DIGITAL  , DrvInputPort3 + 7, "p3 start"  },
	{"Coin 4"            , BIT_DIGITAL  , DrvInputPort0 + 3, "p4 coin"   },
	{"Start 4"           , BIT_DIGITAL  , DrvInputPort4 + 7, "p4 start"  },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort1 + 0, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p1 fire 2" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort2 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort2 + 0, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort2 + 1, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort2 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort2 + 5, "p2 fire 2" },
	
	{"P3 Up"             , BIT_DIGITAL  , DrvInputPort3 + 2, "p3 up"     },
	{"P3 Down"           , BIT_DIGITAL  , DrvInputPort3 + 3, "p3 down"   },
	{"P3 Left"           , BIT_DIGITAL  , DrvInputPort3 + 0, "p3 left"   },
	{"P3 Right"          , BIT_DIGITAL  , DrvInputPort3 + 1, "p3 right"  },
	{"P3 Fire 1"         , BIT_DIGITAL  , DrvInputPort3 + 4, "p3 fire 1" },
	{"P3 Fire 2"         , BIT_DIGITAL  , DrvInputPort3 + 5, "p3 fire 2" },
	
	{"P4 Up"             , BIT_DIGITAL  , DrvInputPort4 + 2, "p4 up"     },
	{"P4 Down"           , BIT_DIGITAL  , DrvInputPort4 + 3, "p4 down"   },
	{"P4 Left"           , BIT_DIGITAL  , DrvInputPort4 + 0, "p4 left"   },
	{"P4 Right"          , BIT_DIGITAL  , DrvInputPort4 + 1, "p4 right"  },
	{"P4 Fire 1"         , BIT_DIGITAL  , DrvInputPort4 + 4, "p4 fire 1" },
	{"P4 Fire 2"         , BIT_DIGITAL  , DrvInputPort4 + 5, "p4 fire 2" },
	
	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service 1"         , BIT_DIGITAL  , DrvInputPort0 + 4, "service"   },
	{"Service 2"         , BIT_DIGITAL  , DrvInputPort0 + 5, "service 2" },
	{"Service 3"         , BIT_DIGITAL  , DrvInputPort0 + 6, "service 3" },
	{"Service 4"         , BIT_DIGITAL  , DrvInputPort0 + 7, "service 4" },
	{"Diagnostics"       , BIT_DIGITAL  , DrvInputPort5 + 7, "diag"      },
};

STDINPUTINFO(Ssriders4ps)

static struct BurnInputInfo Thndrx2InputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvInputPort1 + 0,	"p1 coin"	},

	{"P1 Start",		BIT_DIGITAL,	DrvInputPort0 + 7,	"p1 start"	},
	{"P1 Up",		BIT_DIGITAL,	DrvInputPort0 + 2,	"p1 up"		},
	{"P1 Down",		BIT_DIGITAL,	DrvInputPort0 + 3,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvInputPort0 + 0,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvInputPort0 + 1,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvInputPort0 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvInputPort0 + 5,	"p1 fire 2"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvInputPort1 + 1,	"p2 coin"	},

	{"P2 Start",		BIT_DIGITAL,	DrvInputPort2 + 7,	"p2 start"	},
	{"P2 Up",		BIT_DIGITAL,	DrvInputPort2 + 2,	"p2 up"		},
	{"P2 Down",		BIT_DIGITAL,	DrvInputPort2 + 3,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvInputPort2 + 0,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvInputPort2 + 1,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvInputPort2 + 4,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvInputPort2 + 5,	"p2 fire 2"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",		BIT_DIGITAL,	DrvInputPort1 + 2,	"service"	},
	{"Diagnostics",		BIT_DIGITAL,	DrvInputPort1 + 3,	"diag"		},
};

STDINPUTINFO(Thndrx2)

static struct BurnInputInfo LgtnfghtInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvInputPort0 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvInputPort0 + 3,	"p1 start"	},
	{"P1 Up",		BIT_DIGITAL,	DrvInputPort1 + 2,	"p1 up"		},
	{"P1 Down",		BIT_DIGITAL,	DrvInputPort1 + 3,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvInputPort1 + 0,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvInputPort1 + 1,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvInputPort1 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvInputPort1 + 5,	"p1 fire 2"	},
	{"P1 Button 3",		BIT_DIGITAL,	DrvInputPort1 + 6,	"p1 fire 3"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvInputPort0 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvInputPort0 + 4,	"p2 start"	},
	{"P2 Up",		BIT_DIGITAL,	DrvInputPort2 + 2,	"p2 up"		},
	{"P2 Down",		BIT_DIGITAL,	DrvInputPort2 + 3,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvInputPort2 + 0,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvInputPort2 + 1,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvInputPort2 + 4,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvInputPort2 + 5,	"p2 fire 2"	},
	{"P2 Button 3",		BIT_DIGITAL,	DrvInputPort2 + 6,	"p2 fire 3"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",		BIT_DIGITAL,	DrvInputPort0 + 6,	"service"	},
	{"Dip A",		BIT_DIPSWITCH,	DrvDip + 0,		"dip"		},
	{"Dip B",		BIT_DIPSWITCH,	DrvDip + 1,		"dip"		},
	{"Dip C",		BIT_DIPSWITCH,	DrvDip + 2,		"dip"		},
};

STDINPUTINFO(Lgtnfght)

static struct BurnInputInfo QgakumonInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvInputPort0 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvInputPort1 + 7,	"p1 start"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvInputPort1 + 2,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvInputPort1 + 3,	"p1 fire 2"	},
	{"P1 Button 3",		BIT_DIGITAL,	DrvInputPort1 + 0,	"p1 fire 3"	},
	{"P1 Button 4",		BIT_DIGITAL,	DrvInputPort1 + 1,	"p1 fire 4"	},
	{"P1 Button 5",		BIT_DIGITAL,	DrvInputPort1 + 4,	"p1 fire 5"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvInputPort0 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvInputPort2 + 7,	"p2 start"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvInputPort2 + 2,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvInputPort2 + 3,	"p2 fire 2"	},
	{"P2 Button 3",		BIT_DIGITAL,	DrvInputPort2 + 0,	"p2 fire 3"	},
	{"P2 Button 4",		BIT_DIGITAL,	DrvInputPort2 + 1,	"p2 fire 4"	},
	{"P2 Button 5",		BIT_DIGITAL,	DrvInputPort2 + 4,	"p2 fire 5"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",		BIT_DIGITAL,	DrvInputPort5 + 4,	"service"	},
	{"Diagnostics",		BIT_DIGITAL,	DrvInputPort5 + 7,	"diag"		},
};

STDINPUTINFO(Qgakumon)

static struct BurnInputInfo PunkshotInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvInputPort0 + 0,	"p1 coin"	},
	{"P1 Up",		BIT_DIGITAL,	DrvInputPort1 + 2,	"p1 up"		},
	{"P1 Down",		BIT_DIGITAL,	DrvInputPort1 + 3,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvInputPort1 + 0,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvInputPort1 + 1,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvInputPort1 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvInputPort1 + 5,	"p1 fire 2"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvInputPort0 + 1,	"p2 coin"	},
	{"P2 Up",		BIT_DIGITAL,	DrvInputPort2 + 2,	"p2 up"		},
	{"P2 Down",		BIT_DIGITAL,	DrvInputPort2 + 3,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvInputPort2 + 0,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvInputPort2 + 1,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvInputPort2 + 4,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvInputPort2 + 5,	"p2 fire 2"	},

	{"P3 Coin",		BIT_DIGITAL,	DrvInputPort0 + 2,	"p3 coin"	},
	{"P3 Up",		BIT_DIGITAL,	DrvInputPort3 + 2,	"p3 up"		},
	{"P3 Down",		BIT_DIGITAL,	DrvInputPort3 + 3,	"p3 down"	},
	{"P3 Left",		BIT_DIGITAL,	DrvInputPort3 + 0,	"p3 left"	},
	{"P3 Right",		BIT_DIGITAL,	DrvInputPort3 + 1,	"p3 right"	},
	{"P3 Button 1",		BIT_DIGITAL,	DrvInputPort3 + 4,	"p3 fire 1"	},
	{"P3 Button 2",		BIT_DIGITAL,	DrvInputPort3 + 5,	"p3 fire 2"	},

	{"P4 Coin",		BIT_DIGITAL,	DrvInputPort0 + 3,	"p4 coin"	},
	{"P4 Up",		BIT_DIGITAL,	DrvInputPort4 + 2,	"p4 up"		},
	{"P4 Down",		BIT_DIGITAL,	DrvInputPort4 + 3,	"p4 down"	},
	{"P4 Left",		BIT_DIGITAL,	DrvInputPort4 + 0,	"p4 left"	},
	{"P4 Right",		BIT_DIGITAL,	DrvInputPort4 + 1,	"p4 right"	},
	{"P4 Button 1",		BIT_DIGITAL,	DrvInputPort4 + 4,	"p4 fire 1"	},
	{"P4 Button 2",		BIT_DIGITAL,	DrvInputPort4 + 5,	"p4 fire 2"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",		BIT_DIGITAL,	DrvInputPort0 + 4,	"service"	},
	{"Dip A",		BIT_DIPSWITCH,	DrvDip + 0,		"dip"		},
	{"Dip B",		BIT_DIPSWITCH,	DrvDip + 1,		"dip"		},
	{"Dip C",		BIT_DIPSWITCH,	DrvDip + 2,		"dip"		},
};

STDINPUTINFO(Punkshot)

static struct BurnInputInfo Punksht2InputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvInputPort0 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvInputPort5 + 0,	"p1 start"	},
	{"P1 Up",		BIT_DIGITAL,	DrvInputPort1 + 2,	"p1 up"		},
	{"P1 Down",		BIT_DIGITAL,	DrvInputPort1 + 3,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvInputPort1 + 0,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvInputPort1 + 1,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvInputPort1 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvInputPort1 + 5,	"p1 fire 2"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvInputPort0 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvInputPort5 + 1,	"p2 start"	},
	{"P2 Up",		BIT_DIGITAL,	DrvInputPort2 + 2,	"p2 up"		},
	{"P2 Down",		BIT_DIGITAL,	DrvInputPort2 + 3,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvInputPort2 + 0,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvInputPort2 + 1,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvInputPort2 + 4,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvInputPort2 + 5,	"p2 fire 2"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",		BIT_DIGITAL,	DrvInputPort0 + 4,	"service"	},
	{"Dip A",		BIT_DIPSWITCH,	DrvDip + 0,		"dip"		},
	{"Dip B",		BIT_DIPSWITCH,	DrvDip + 1,		"dip"		},
	{"Dip C",		BIT_DIPSWITCH,	DrvDip + 2,		"dip"		},
};

STDINPUTINFO(Punksht2)

static inline void DrvClearOpposites(unsigned char* nJoystickInputs)
{
	if ((*nJoystickInputs & 0x03) == 0x03) {
		*nJoystickInputs &= ~0x03;
	}
	if ((*nJoystickInputs & 0x0c) == 0x0c) {
		*nJoystickInputs &= ~0x0c;
	}
}

static inline void DrvMakeInputs()
{
	// Reset Inputs
	DrvInput[0] = DrvInput[1] = DrvInput[2] = DrvInput[3] = DrvInput[4] = DrvInput[5] = 0x00;

	// Compile Digital Inputs
	for (int i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvInputPort0[i] & 1) << i;
		DrvInput[1] |= (DrvInputPort1[i] & 1) << i;
		DrvInput[2] |= (DrvInputPort2[i] & 1) << i;
		DrvInput[3] |= (DrvInputPort3[i] & 1) << i;
		DrvInput[4] |= (DrvInputPort3[i] & 1) << i;
		DrvInput[5] |= (DrvInputPort5[i] & 1) << i;
	}

	// Clear Opposites
	DrvClearOpposites(&DrvInput[1]);
	DrvClearOpposites(&DrvInput[2]);
	DrvClearOpposites(&DrvInput[3]);
	DrvClearOpposites(&DrvInput[4]);
}

static inline void SsridersMakeInputs()
{
	DrvMakeInputs();
	
	DrvInput[5] ^= 0xf6;
	//if (DrvInputPort5[7]) DrvInput[5] -= 0x80;
}

static struct BurnDIPInfo TmntDIPList[]=
{
	// Default Values
	{0x21, 0xff, 0xff, 0xff, NULL                     },
	{0x22, 0xff, 0xff, 0x5f, NULL                     },
	{0x23, 0xff, 0xff, 0xff, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 16   , "Coinage"               },
	{0x21, 0x01, 0x0f, 0x00, "5 Coins 1 Credit"       },
	{0x21, 0x01, 0x0f, 0x02, "4 Coins 1 Credit"       },
	{0x21, 0x01, 0x0f, 0x05, "3 Coins 1 Credit"       },
	{0x21, 0x01, 0x0f, 0x08, "2 Coins 1 Credit"       },
	{0x21, 0x01, 0x0f, 0x04, "3 Coins 2 Credits"      },
	{0x21, 0x01, 0x0f, 0x01, "4 Coins 3 Credits"      },
	{0x21, 0x01, 0x0f, 0x0f, "1 Coin  1 Credit"       },
	{0x21, 0x01, 0x0f, 0x03, "3 Coins 4 Credits"      },
	{0x21, 0x01, 0x0f, 0x07, "2 Coins 3 Credits"      },
	{0x21, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"      },
	{0x21, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"      },
	{0x21, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"      },
	{0x21, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"      },
	{0x21, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"      },
	{0x21, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"      },
	{0x21, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"      },

	// Dip 2
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x22, 0x01, 0x03, 0x03, "1"                      },
	{0x22, 0x01, 0x03, 0x02, "2"                      },
	{0x22, 0x01, 0x03, 0x01, "3"                      },
	{0x22, 0x01, 0x03, 0x00, "5"                      },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x22, 0x01, 0x60, 0x60, "Easy"                   },
	{0x22, 0x01, 0x60, 0x40, "Normal"                 },
	{0x22, 0x01, 0x60, 0x20, "Difficult"              },
	{0x22, 0x01, 0x60, 0x00, "Very Difficult"         },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x22, 0x01, 0x80, 0x80, "Off"                    },
	{0x22, 0x01, 0x80, 0x00, "On"                     },
	
	// Dip 3
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x23, 0x01, 0x01, 0x01, "Off"                    },
	{0x23, 0x01, 0x01, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Test Mode"              },
	{0x23, 0x01, 0x04, 0x04, "Off"                    },
	{0x23, 0x01, 0x04, 0x00, "On"                     },
};

STDDIPINFO(Tmnt)

static struct BurnDIPInfo Tmnt2pDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL                     },
	{0x14, 0xff, 0xff, 0x5f, NULL                     },
	{0x15, 0xff, 0xff, 0xff, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 16   , "Coin A"                },
	{0x13, 0x01, 0x0f, 0x02, "4 Coins 1 Credit"       },
	{0x13, 0x01, 0x0f, 0x05, "3 Coins 1 Credit"       },
	{0x13, 0x01, 0x0f, 0x08, "2 Coins 1 Credit"       },
	{0x13, 0x01, 0x0f, 0x04, "3 Coins 2 Credits"      },
	{0x13, 0x01, 0x0f, 0x01, "4 Coins 3 Credits"      },
	{0x13, 0x01, 0x0f, 0x0f, "1 Coin  1 Credit"       },
	{0x13, 0x01, 0x0f, 0x03, "3 Coins 4 Credits"      },
	{0x13, 0x01, 0x0f, 0x07, "2 Coins 3 Credits"      },
	{0x13, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"      },
	{0x13, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"      },
	{0x13, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"      },
	{0x13, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"      },
	{0x13, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"      },
	{0x13, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"      },
	{0x13, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"      },
	{0x13, 0x01, 0x0f, 0x00, "Free Play"              },
	
	{0   , 0xfe, 0   , 15   , "Coin B"                },
	{0x13, 0x01, 0x0f, 0x20, "4 Coins 1 Credit"       },
	{0x13, 0x01, 0x0f, 0x50, "3 Coins 1 Credit"       },
	{0x13, 0x01, 0x0f, 0x80, "2 Coins 1 Credit"       },
	{0x13, 0x01, 0x0f, 0x40, "3 Coins 2 Credits"      },
	{0x13, 0x01, 0x0f, 0x10, "4 Coins 3 Credits"      },
	{0x13, 0x01, 0x0f, 0xf0, "1 Coin  1 Credit"       },
	{0x13, 0x01, 0x0f, 0x30, "3 Coins 4 Credits"      },
	{0x13, 0x01, 0x0f, 0x70, "2 Coins 3 Credits"      },
	{0x13, 0x01, 0x0f, 0xe0, "1 Coin  2 Credits"      },
	{0x13, 0x01, 0x0f, 0x60, "2 Coins 5 Credits"      },
	{0x13, 0x01, 0x0f, 0xd0, "1 Coin  3 Credits"      },
	{0x13, 0x01, 0x0f, 0xc0, "1 Coin  4 Credits"      },
	{0x13, 0x01, 0x0f, 0xb0, "1 Coin  5 Credits"      },
	{0x13, 0x01, 0x0f, 0xa0, "1 Coin  6 Credits"      },
	{0x13, 0x01, 0x0f, 0x90, "1 Coin  7 Credits"      },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x14, 0x01, 0x03, 0x03, "1"                      },
	{0x14, 0x01, 0x03, 0x02, "2"                      },
	{0x14, 0x01, 0x03, 0x01, "3"                      },
	{0x14, 0x01, 0x03, 0x00, "5"                      },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x14, 0x01, 0x60, 0x60, "Easy"                   },
	{0x14, 0x01, 0x60, 0x40, "Normal"                 },
	{0x14, 0x01, 0x60, 0x20, "Difficult"              },
	{0x14, 0x01, 0x60, 0x00, "Very Difficult"         },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x14, 0x01, 0x80, 0x80, "Off"                    },
	{0x14, 0x01, 0x80, 0x00, "On"                     },
	
	// Dip 3
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x15, 0x01, 0x01, 0x01, "Off"                    },
	{0x15, 0x01, 0x01, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Test Mode"              },
	{0x15, 0x01, 0x04, 0x04, "Off"                    },
	{0x15, 0x01, 0x04, 0x00, "On"                     },
};

STDDIPINFO(Tmnt2p)

static struct BurnDIPInfo MiaDIPList[]=
{
	// Default Values
	{0x14, 0xff, 0xff, 0xff, NULL                     },
	{0x15, 0xff, 0xff, 0x56, NULL                     },
	{0x16, 0xff, 0xff, 0xff, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 16   , "Coin A"                },
	{0x14, 0x01, 0x0f, 0x02, "4 Coins 1 Credit"       },
	{0x14, 0x01, 0x0f, 0x05, "3 Coins 1 Credit"       },
	{0x14, 0x01, 0x0f, 0x08, "2 Coins 1 Credit"       },
	{0x14, 0x01, 0x0f, 0x04, "3 Coins 2 Credits"      },
	{0x14, 0x01, 0x0f, 0x01, "4 Coins 3 Credits"      },
	{0x14, 0x01, 0x0f, 0x0f, "1 Coin  1 Credit"       },
	{0x14, 0x01, 0x0f, 0x03, "3 Coins 4 Credits"      },
	{0x14, 0x01, 0x0f, 0x07, "2 Coins 3 Credits"      },
	{0x14, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"      },
	{0x14, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"      },
	{0x14, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"      },
	{0x14, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"      },
	{0x14, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"      },
	{0x14, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"      },
	{0x14, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"      },
	{0x14, 0x01, 0x0f, 0x00, "Free Play"              },
	
	{0   , 0xfe, 0   , 15   , "Coin B"                },
	{0x14, 0x01, 0x0f, 0x20, "4 Coins 1 Credit"       },
	{0x14, 0x01, 0x0f, 0x50, "3 Coins 1 Credit"       },
	{0x14, 0x01, 0x0f, 0x80, "2 Coins 1 Credit"       },
	{0x14, 0x01, 0x0f, 0x40, "3 Coins 2 Credits"      },
	{0x14, 0x01, 0x0f, 0x10, "4 Coins 3 Credits"      },
	{0x14, 0x01, 0x0f, 0xf0, "1 Coin  1 Credit"       },
	{0x14, 0x01, 0x0f, 0x30, "3 Coins 4 Credits"      },
	{0x14, 0x01, 0x0f, 0x70, "2 Coins 3 Credits"      },
	{0x14, 0x01, 0x0f, 0xe0, "1 Coin  2 Credits"      },
	{0x14, 0x01, 0x0f, 0x60, "2 Coins 5 Credits"      },
	{0x14, 0x01, 0x0f, 0xd0, "1 Coin  3 Credits"      },
	{0x14, 0x01, 0x0f, 0xc0, "1 Coin  4 Credits"      },
	{0x14, 0x01, 0x0f, 0xb0, "1 Coin  5 Credits"      },
	{0x14, 0x01, 0x0f, 0xa0, "1 Coin  6 Credits"      },
	{0x14, 0x01, 0x0f, 0x90, "1 Coin  7 Credits"      },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x15, 0x01, 0x03, 0x03, "2"                      },
	{0x15, 0x01, 0x03, 0x02, "3"                      },
	{0x15, 0x01, 0x03, 0x01, "5"                      },
	{0x15, 0x01, 0x03, 0x00, "7"                      },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x15, 0x01, 0x18, 0x18, "30k  80k"               },
	{0x15, 0x01, 0x18, 0x10, "50k 100k"               },
	{0x15, 0x01, 0x18, 0x08, "50k"                    },
	{0x15, 0x01, 0x18, 0x00, "100k"                   },
	
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x15, 0x01, 0x60, 0x60, "Easy"                   },
	{0x15, 0x01, 0x60, 0x40, "Normal"                 },
	{0x15, 0x01, 0x60, 0x20, "Difficult"              },
	{0x15, 0x01, 0x60, 0x00, "Very Difficult"         },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x15, 0x01, 0x80, 0x80, "Off"                    },
	{0x15, 0x01, 0x80, 0x00, "On"                     },
	
	// Dip 3
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x16, 0x01, 0x01, 0x01, "Off"                    },
	{0x16, 0x01, 0x01, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "VRAM Character Check"   },
	{0x16, 0x01, 0x02, 0x02, "Off"                    },
	{0x16, 0x01, 0x02, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Test Mode"              },
	{0x16, 0x01, 0x04, 0x04, "Off"                    },
	{0x16, 0x01, 0x04, 0x00, "On"                     },
};

STDDIPINFO(Mia)

static struct BurnDIPInfo LgtnfghtDIPList[]=
{
	{0x14, 0xff, 0xff, 0x5e, NULL			},
	{0x15, 0xff, 0xff, 0xff, NULL			},
	{0x16, 0xff, 0xff, 0xfd, NULL			},

	{0   , 0xfe, 0   ,    4, "Lives"		},
	{0x14, 0x01, 0x03, 0x03, "2"			},
	{0x14, 0x01, 0x03, 0x02, "3"			},
	{0x14, 0x01, 0x03, 0x01, "5"			},
	{0x14, 0x01, 0x03, 0x00, "7"			},

	{0   , 0xfe, 0   ,    4, "Bonus Life"		},
	{0x14, 0x01, 0x18, 0x18, "100000 400000"	},
	{0x14, 0x01, 0x18, 0x10, "150000 500000"	},
	{0x14, 0x01, 0x18, 0x08, "200000"		},
	{0x14, 0x01, 0x18, 0x00, "None"			},

	{0   , 0xfe, 0   ,    4, "Difficulty"		},
	{0x14, 0x01, 0x60, 0x60, "Easy"			},
	{0x14, 0x01, 0x60, 0x40, "Medium"		},
	{0x14, 0x01, 0x60, 0x20, "Hard"			},
	{0x14, 0x01, 0x60, 0x00, "Hardest"		},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x14, 0x01, 0x80, 0x80, "Off"			},
	{0x14, 0x01, 0x80, 0x00, "On"			},

	{0   , 0xfe, 0   ,    16, "Coin A"		},
	{0x15, 0x01, 0x0f, 0x02, "4 Coins 1 Credits"	},
	{0x15, 0x01, 0x0f, 0x05, "3 Coins 1 Credits"	},
	{0x15, 0x01, 0x0f, 0x08, "2 Coins 1 Credits"	},
	{0x15, 0x01, 0x0f, 0x04, "3 Coins 2 Credits"	},
	{0x15, 0x01, 0x0f, 0x01, "4 Coins 3 Credits"	},
	{0x15, 0x01, 0x0f, 0x0f, "1 Coin  1 Credits"	},
	{0x15, 0x01, 0x0f, 0x03, "3 Coins 4 Credits"	},
	{0x15, 0x01, 0x0f, 0x07, "2 Coins 3 Credits"	},
	{0x15, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"	},
	{0x15, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"	},
	{0x15, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"	},
	{0x15, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"	},
	{0x15, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"	},
	{0x15, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"	},
	{0x15, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"	},
	{0x15, 0x01, 0x0f, 0x00, "Free Play"		},

	{0   , 0xfe, 0   ,    16, "Coin B"		},
	{0x15, 0x01, 0xf0, 0x20, "4 Coins 1 Credits"	},
	{0x15, 0x01, 0xf0, 0x50, "3 Coins 1 Credits"	},
	{0x15, 0x01, 0xf0, 0x80, "2 Coins 1 Credits"	},
	{0x15, 0x01, 0xf0, 0x40, "3 Coins 2 Credits"	},
	{0x15, 0x01, 0xf0, 0x10, "4 Coins 3 Credits"	},
	{0x15, 0x01, 0xf0, 0xf0, "1 Coin  1 Credits"	},
	{0x15, 0x01, 0xf0, 0x30, "3 Coins 4 Credits"	},
	{0x15, 0x01, 0xf0, 0x70, "2 Coins 3 Credits"	},
	{0x15, 0x01, 0xf0, 0xe0, "1 Coin  2 Credits"	},
	{0x15, 0x01, 0xf0, 0x60, "2 Coins 5 Credits"	},
	{0x15, 0x01, 0xf0, 0xd0, "1 Coin  3 Credits"	},
	{0x15, 0x01, 0xf0, 0xc0, "1 Coin  4 Credits"	},
	{0x15, 0x01, 0xf0, 0xb0, "1 Coin  5 Credits"	},
	{0x15, 0x01, 0xf0, 0xa0, "1 Coin  6 Credits"	},
	{0x15, 0x01, 0xf0, 0x90, "1 Coin  7 Credits"	},
	{0x15, 0x01, 0xf0, 0x00, "No Coin B"		},

	{0   , 0xfe, 0   ,    2, "Flip Screen"		},
	{0x16, 0x01, 0x01, 0x01, "Off"			},
	{0x16, 0x01, 0x01, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Sound"		},
	{0x16, 0x01, 0x02, 0x02, "Mono"			},
	{0x16, 0x01, 0x02, 0x00, "Stereo"		},

	{0   , 0xfe, 0   ,    2, "Service Mode"		},
	{0x16, 0x01, 0x04, 0x04, "Off"			},
	{0x16, 0x01, 0x04, 0x00, "On"			},
};

STDDIPINFO(Lgtnfght)

static struct BurnDIPInfo PunkshotDIPList[]=
{
	{0x1e, 0xff, 0xff, 0xff, NULL			},
	{0x1f, 0xff, 0xff, 0x7f, NULL			},
	{0x20, 0xff, 0xff, 0xf0, NULL			},

	{0   , 0xfe, 0   ,   16, "Coinage"		},
	{0x1e, 0x01, 0x0f, 0x00, "5 Coins 1 Credits"	},
	{0x1e, 0x01, 0x0f, 0x02, "4 Coins 1 Credits"	},
	{0x1e, 0x01, 0x0f, 0x05, "3 Coins 1 Credits"	},
	{0x1e, 0x01, 0x0f, 0x08, "2 Coins 1 Credits"	},
	{0x1e, 0x01, 0x0f, 0x04, "3 Coins 2 Credits"	},
	{0x1e, 0x01, 0x0f, 0x01, "4 Coins 3 Credits"	},
	{0x1e, 0x01, 0x0f, 0x0f, "1 Coin  1 Credits"	},
	{0x1e, 0x01, 0x0f, 0x03, "3 Coins 4 Credits"	},
	{0x1e, 0x01, 0x0f, 0x07, "2 Coins 3 Credits"	},
	{0x1e, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"	},
	{0x1e, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"	},
	{0x1e, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"	},
	{0x1e, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"	},
	{0x1e, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"	},
	{0x1e, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"	},
	{0x1e, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"	},

	{0   , 0xfe, 0   ,    2, "Continue"		},
	{0x1e, 0x01, 0x10, 0x10, "Normal"		},
	{0x1e, 0x01, 0x10, 0x00, "1 Coin"		},

	{0   , 0xfe, 0   ,    4, "Energy"		},
	{0x1f, 0x01, 0x03, 0x03, "30"			},
	{0x1f, 0x01, 0x03, 0x02, "40"			},
	{0x1f, 0x01, 0x03, 0x01, "50"			},
	{0x1f, 0x01, 0x03, 0x00, "60"			},

	{0   , 0xfe, 0   ,    4, "Period Length"	},
	{0x1f, 0x01, 0x0c, 0x0c, "2 Minutes"		},
	{0x1f, 0x01, 0x0c, 0x08, "3 Minutes"		},
	{0x1f, 0x01, 0x0c, 0x04, "4 Minutes"		},
	{0x1f, 0x01, 0x0c, 0x00, "5 Minutes"		},

	{0   , 0xfe, 0   ,    4, "Difficulty"		},
	{0x1f, 0x01, 0x60, 0x60, "Easy"			},
	{0x1f, 0x01, 0x60, 0x40, "Medium"		},
	{0x1f, 0x01, 0x60, 0x20, "Hard"			},
	{0x1f, 0x01, 0x60, 0x00, "Hardest"		},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x1f, 0x01, 0x80, 0x80, "Off"			},
	{0x1f, 0x01, 0x80, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Flip Screen"		},
	{0x20, 0x01, 0x10, 0x10, "Off"			},
	{0x20, 0x01, 0x10, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Service Mode"		},
	{0x20, 0x01, 0x40, 0x40, "Off"			},
	{0x20, 0x01, 0x40, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Freeze"		},
	{0x20, 0x01, 0x80, 0x80, "Off"			},
	{0x20, 0x01, 0x80, 0x00, "On"			},
};

STDDIPINFO(Punkshot)

static struct BurnDIPInfo Punksht2DIPList[]=
{
	{0x12, 0xff, 0xff, 0xff, NULL			},
	{0x13, 0xff, 0xff, 0x7f, NULL			},
	{0x14, 0xff, 0xff, 0xf0, NULL			},

	{0   , 0xfe, 0   ,    16, "Coinage"		},
	{0x12, 0x01, 0x0f, 0x00, "5 Coins 1 Credits"	},
	{0x12, 0x01, 0x0f, 0x02, "4 Coins 1 Credits"	},
	{0x12, 0x01, 0x0f, 0x05, "3 Coins 1 Credits"	},
	{0x12, 0x01, 0x0f, 0x08, "2 Coins 1 Credits"	},
	{0x12, 0x01, 0x0f, 0x04, "3 Coins 2 Credits"	},
	{0x12, 0x01, 0x0f, 0x01, "4 Coins 3 Credits"	},
	{0x12, 0x01, 0x0f, 0x0f, "1 Coin  1 Credits"	},
	{0x12, 0x01, 0x0f, 0x03, "3 Coins 4 Credits"	},
	{0x12, 0x01, 0x0f, 0x07, "2 Coins 3 Credits"	},
	{0x12, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"	},
	{0x12, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"	},
	{0x12, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"	},
	{0x12, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"	},
	{0x12, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"	},
	{0x12, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"	},
	{0x12, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"	},

	{0   , 0xfe, 0   ,    2, "Continue"		},
	{0x12, 0x01, 0x10, 0x10, "Normal"		},
	{0x12, 0x01, 0x10, 0x00, "1 Coin"		},

	{0   , 0xfe, 0   ,    4, "Energy"		},
	{0x13, 0x01, 0x03, 0x03, "40"			},
	{0x13, 0x01, 0x03, 0x02, "50"			},
	{0x13, 0x01, 0x03, 0x01, "60"			},
	{0x13, 0x01, 0x03, 0x00, "70"			},

	{0   , 0xfe, 0   ,    4, "Period Length"	},
	{0x13, 0x01, 0x0c, 0x0c, "3 Minutes"		},
	{0x13, 0x01, 0x0c, 0x08, "4 Minutes"		},
	{0x13, 0x01, 0x0c, 0x04, "5 Minutes"		},
	{0x13, 0x01, 0x0c, 0x00, "6 Minutes"		},

	{0   , 0xfe, 0   ,    4, "Difficulty"		},
	{0x13, 0x01, 0x60, 0x60, "Easy"			},
	{0x13, 0x01, 0x60, 0x40, "Medium"		},
	{0x13, 0x01, 0x60, 0x20, "Hard"			},
	{0x13, 0x01, 0x60, 0x00, "Hardest"		},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x13, 0x01, 0x80, 0x80, "Off"			},
	{0x13, 0x01, 0x80, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Flip Screen"		},
	{0x14, 0x01, 0x10, 0x10, "Off"			},
	{0x14, 0x01, 0x10, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Service Mode"		},
	{0x14, 0x01, 0x40, 0x40, "Off"			},
	{0x14, 0x01, 0x40, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Freeze"		},
	{0x14, 0x01, 0x80, 0x80, "Off"			},
	{0x14, 0x01, 0x80, 0x00, "On"			},
};

STDDIPINFO(Punksht2)

static struct BurnRomInfo TmntRomDesc[] = {
	{ "963-x23.j17",        0x020000, 0xa9549004, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963-x24.k17",        0x020000, 0xe5cc9067, BRF_ESS | BRF_PRG }, //  1
	{ "963-x21.j15",        0x010000, 0x5789cf92, BRF_ESS | BRF_PRG }, //  2
	{ "963-x22.k15",        0x010000, 0x0a74e277, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmnt)
STD_ROM_FN(Tmnt)

static struct BurnRomInfo TmntuRomDesc[] = {
	{ "963-r23.j17",        0x020000, 0xa7f61195, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963-r24.k17",        0x020000, 0x661e056a, BRF_ESS | BRF_PRG }, //  1
	{ "963-r21.j15",        0x010000, 0xde047bb6, BRF_ESS | BRF_PRG }, //  2
	{ "963-r22.k15",        0x010000, 0xd86a0888, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmntu)
STD_ROM_FN(Tmntu)

static struct BurnRomInfo TmntuaRomDesc[] = {
	{ "963-j23.j17",        0x020000, 0xf77314e2, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963-j24.k17",        0x020000, 0x47f662d3, BRF_ESS | BRF_PRG }, //  1
	{ "963-j21.j15",        0x010000, 0x7bee9fe8, BRF_ESS | BRF_PRG }, //  2
	{ "963-j22.k15",        0x010000, 0x2efed09f, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmntua)
STD_ROM_FN(Tmntua)

static struct BurnRomInfo TmhtRomDesc[] = {
	{ "963-f23.j17",        0x020000, 0x9cb5e461, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963-f24.k17",        0x020000, 0x2d902fab, BRF_ESS | BRF_PRG }, //  1
	{ "963-f21.j15",        0x010000, 0x9fa25378, BRF_ESS | BRF_PRG }, //  2
	{ "963-f22.k15",        0x010000, 0x2127ee53, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmht)
STD_ROM_FN(Tmht)

static struct BurnRomInfo TmntjRomDesc[] = {
	{ "963_223.j17",        0x020000, 0x0d34a5ff, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963_224.k17",        0x020000, 0x2fd453f2, BRF_ESS | BRF_PRG }, //  1
	{ "963_221.j15",        0x010000, 0xfa8e25fd, BRF_ESS | BRF_PRG }, //  2
	{ "963_222.k15",        0x010000, 0xca437a4f, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmntj)
STD_ROM_FN(Tmntj)

static struct BurnRomInfo Tmht2pRomDesc[] = {
	{ "963-u23.j17",        0x020000, 0x58bec748, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963-u24.k17",        0x020000, 0xdce87c8d, BRF_ESS | BRF_PRG }, //  1
	{ "963-u21.j15",        0x010000, 0xabce5ead, BRF_ESS | BRF_PRG }, //  2
	{ "963-u22.k15",        0x010000, 0x4ecc8d6b, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmht2p)
STD_ROM_FN(Tmht2p)

static struct BurnRomInfo Tmht2paRomDesc[] = {
	{ "963-_23.j17",        0x020000, 0x8698061a, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963-_24.k17",        0x020000, 0x4036c075, BRF_ESS | BRF_PRG }, //  1
	{ "963-_21.j15",        0x010000, 0xddcc979c, BRF_ESS | BRF_PRG }, //  2
	{ "963-_22.k15",        0x010000, 0x71a38d27, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmht2pa)
STD_ROM_FN(Tmht2pa)

static struct BurnRomInfo Tmnt2pjRomDesc[] = {
	{ "963-123.j17",        0x020000, 0x6a3527c9, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "963-124.k17",        0x020000, 0x2c4bfa15, BRF_ESS | BRF_PRG }, //  1
	{ "963-121.j15",        0x010000, 0x4181b733, BRF_ESS | BRF_PRG }, //  2
	{ "963-122.k15",        0x010000, 0xc64eb5ff, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmnt2pj)
STD_ROM_FN(Tmnt2pj)

static struct BurnRomInfo Tmnt2poRomDesc[] = {
	{ "tmnt123.j17",        0x020000, 0x2d905183, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "tmnt124.k17",        0x020000, 0xe0125352, BRF_ESS | BRF_PRG }, //  1
	{ "tmnt21.j15",         0x010000, 0x12deeafb, BRF_ESS | BRF_PRG }, //  2
	{ "tmnt22.k15",         0x010000, 0xaec4f1c3, BRF_ESS | BRF_PRG }, //  3
	
	{ "963e20.g13",         0x008000, 0x1692a6d6, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "963a28.h27",         0x080000, 0xdb4769a8, BRF_GRA },	   //  5	Tiles
	{ "963a29.k27",         0x080000, 0x8069cd2e, BRF_GRA },	   //  6
	
	{ "963a17.h4",          0x080000, 0xb5239a44, BRF_GRA },	   //  7	Sprites
	{ "963a18.h6",          0x080000, 0xdd51adef, BRF_GRA },	   //  8
	{ "963a15.k4",          0x080000, 0x1f324eed, BRF_GRA },	   //  9
	{ "963a16.k6",          0x080000, 0xd4bd9984, BRF_GRA },	   //  10
	
	{ "963a30.g7",          0x000100, 0xabd82680, BRF_GRA },	   //  11	PROMs
	{ "963a31.g19",         0x000100, 0xf8004a1c, BRF_GRA },	   //  12
	
	{ "963a26.c13",         0x020000, 0xe2ac3063, BRF_SND },	   //  13	K007232 Samples
	
	{ "963a27.d18",         0x020000, 0x2dfd674b, BRF_SND },	   //  14	UP7759C Samples
	
	{ "963a25.d5",          0x080000, 0xfca078c7, BRF_SND },	   //  15	Title Music Sample
};

STD_ROM_PICK(Tmnt2po)
STD_ROM_FN(Tmnt2po)

static struct BurnRomInfo MiaRomDesc[] = {
	{ "808t20.h17",         0x020000, 0x6f0acb1d, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "808t21.j17",         0x020000, 0x42a30416, BRF_ESS | BRF_PRG }, //  1
	
	{ "808e03.f4",          0x008000, 0x3d93a7cd, BRF_ESS | BRF_PRG }, //  2	Z80 Program 
	
	{ "808e12.f28",         0x010000, 0xd62f1fde, BRF_GRA },	   //  3	Tiles
	{ "808e13.h28",         0x010000, 0x1fa708f4, BRF_GRA },	   //  4
	{ "808e22.i28",         0x010000, 0x73d758f6, BRF_GRA },	   //  5
	{ "808e23.k28",         0x010000, 0x8ff08b21, BRF_GRA },	   //  6
	
	{ "808d17.j4",          0x080000, 0xd1299082, BRF_GRA },	   //  7	Sprites
	{ "808d15.h4",          0x080000, 0x2b22a6b6, BRF_GRA },	   //  8
	
	{ "808a18.f16",         0x000100, 0xeb95aede, BRF_GRA },	   //  9	PROMs
	
	{ "808d01.d4",          0x020000, 0xfd4d37c0, BRF_SND },	   //  10	K007232 Samples
};

STD_ROM_PICK(Mia)
STD_ROM_FN(Mia)

static struct BurnRomInfo Mia2RomDesc[] = {
	{ "808s20.h17",         0x020000, 0xcaa2897f, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "808s21.j17",         0x020000, 0x3d892ffb, BRF_ESS | BRF_PRG }, //  1
	
	{ "808e03.f4",          0x008000, 0x3d93a7cd, BRF_ESS | BRF_PRG }, //  2	Z80 Program 
	
	{ "808e12.f28",         0x010000, 0xd62f1fde, BRF_GRA },	   //  3	Tiles
	{ "808e13.h28",         0x010000, 0x1fa708f4, BRF_GRA },	   //  4
	{ "808e22.i28",         0x010000, 0x73d758f6, BRF_GRA },	   //  5
	{ "808e23.k28",         0x010000, 0x8ff08b21, BRF_GRA },	   //  6
	
	{ "808d17.j4",          0x080000, 0xd1299082, BRF_GRA },	   //  7	Sprites
	{ "808d15.h4",          0x080000, 0x2b22a6b6, BRF_GRA },	   //  8
	
	{ "808a18.f16",         0x000100, 0xeb95aede, BRF_GRA },	   //  9	PROMs
	
	{ "808d01.d4",          0x020000, 0xfd4d37c0, BRF_SND },	   //  10	K007232 Samples
};

STD_ROM_PICK(Mia2)
STD_ROM_FN(Mia2)

static struct BurnRomInfo CuebrickRomDesc[] = {
	{ "903d25.g12",         0x010000, 0x8d575663, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "903d24.f12",         0x010000, 0x2973625d, BRF_ESS | BRF_PRG }, //  1
	
	{ "903c29.k21",         0x010000, 0xfada986d, BRF_GRA },	   //  2	Tiles
	{ "903c27.k17",         0x010000, 0x5bd4b8e1, BRF_GRA },	   //  3
	{ "903c28.k19",         0x010000, 0x80d2bfaf, BRF_GRA },	   //  4
	{ "903c26.k15",         0x010000, 0xf808fa3d, BRF_GRA },	   //  5
	
	{ "903d23.k12",         0x010000, 0xc39fc9fd, BRF_GRA },	   //  6	Sprites
	{ "903d21.k8",          0x010000, 0x3c7bf8cd, BRF_GRA },	   //  7
	{ "903d22.k10",         0x010000, 0x95ad8591, BRF_GRA },	   //  8
	{ "903d20.k6",          0x010000, 0x2872a1bb, BRF_GRA },	   //  9
};

STD_ROM_PICK(Cuebrick)
STD_ROM_FN(Cuebrick)

static struct BurnRomInfo BlswhstlRomDesc[] = {
	{ "060_l02.e09",        0x020000, 0xe8b7b234, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "060_l03.g09",        0x020000, 0x3c26d281, BRF_ESS | BRF_PRG }, //  1
	{ "060_l09.e11",        0x020000, 0x14628736, BRF_ESS | BRF_PRG }, //  2
	{ "060_l10.g11",        0x020000, 0xf738ad4a, BRF_ESS | BRF_PRG }, //  3
	
	{ "060j01.f3",          0x010000, 0xf9d9a673, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "060e07.k16",         0x080000, 0xc400edf3, BRF_GRA },	   //  5	Tiles
	{ "060e08.k12",         0x080000, 0x70dddba1, BRF_GRA },	   //  6
	
	{ "060e06.k7",          0x080000, 0x09381492, BRF_GRA },	   //  7	Sprites
	{ "060e05.k3",          0x080000, 0x32454241, BRF_GRA },	   //  8
	
	{ "060e04.d1",          0x100000, 0xc680395d, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Blswhstl)
STD_ROM_FN(Blswhstl)

static struct BurnRomInfo DetatwinRomDesc[] = {
	{ "060_j02.e09",        0x020000, 0x11b761ac, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "060_j03.g09",        0x020000, 0x8d0b588c, BRF_ESS | BRF_PRG }, //  1
	{ "060_j09.e11",        0x020000, 0xf2a5f15f, BRF_ESS | BRF_PRG }, //  2
	{ "060_j10.g11",        0x020000, 0x36eefdbc, BRF_ESS | BRF_PRG }, //  3
	
	{ "060j01.f3",          0x010000, 0xf9d9a673, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "060e07.k16",         0x080000, 0xc400edf3, BRF_GRA },	   //  5	Tiles
	{ "060e08.k12",         0x080000, 0x70dddba1, BRF_GRA },	   //  6
	
	{ "060e06.k7",          0x080000, 0x09381492, BRF_GRA },	   //  7	Sprites
	{ "060e05.k3",          0x080000, 0x32454241, BRF_GRA },	   //  8
	
	{ "060e04.d1",          0x100000, 0xc680395d, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Detatwin)
STD_ROM_FN(Detatwin)

static struct BurnRomInfo SsridersRomDesc[] = {
	{ "064eac02.8e",        0x040000, 0x5a5425f4, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064eac03.8g",        0x040000, 0x093c00fb, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssriders)
STD_ROM_FN(Ssriders)

static struct BurnRomInfo SsrdrebdRomDesc[] = {
	{ "064ebd02.8e",        0x040000, 0x8deef9ac, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064ebd03.8g",        0x040000, 0x2370c107, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdrebd)
STD_ROM_FN(Ssrdrebd)

static struct BurnRomInfo SsrdrebcRomDesc[] = {
	{ "064ebc02.8e",        0x040000, 0x9bd7d164, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064ebc03.8g",        0x040000, 0x40fd4165, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdrebc)
STD_ROM_FN(Ssrdrebc)

static struct BurnRomInfo SsrdrudaRomDesc[] = {
	{ "064uda02.8e",        0x040000, 0x5129a6b7, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064uda03.8g",        0x040000, 0x9f887214, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdruda)
STD_ROM_FN(Ssrdruda)

static struct BurnRomInfo SsrdreaaRomDesc[] = {
	{ "064eaa02.8e",        0x040000, 0x4844660f, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064eaa03.8g",        0x040000, 0x0b9bcc7c, BRF_ESS | BRF_PRG }, //  1
	{ "064eaa04.10e",       0x020000, 0x5d917c1c, BRF_ESS | BRF_PRG }, //  2
	{ "064eaa05.10g",       0x020000, 0xf4647b74, BRF_ESS | BRF_PRG }, //  3
	
	{ "064eaa01.2f",        0x010000, 0xbce45d82, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdreaa)
STD_ROM_FN(Ssrdreaa)

static struct BurnRomInfo SsrdruacRomDesc[] = {
	{ "064uac02.8e",        0x040000, 0x870473b6, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064uac03.8g",        0x040000, 0xeadf289a, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdruac)
STD_ROM_FN(Ssrdruac)

static struct BurnRomInfo SsrdrubcRomDesc[] = {
	{ "064ubc02.8e",        0x040000, 0xaca7fda5, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064ubc03.8g",        0x040000, 0xbb1fdeff, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdrubc)
STD_ROM_FN(Ssrdrubc)

static struct BurnRomInfo SsrdrabdRomDesc[] = {
	{ "064abd02.8e",        0x040000, 0x713406cb, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064abd03.8g",        0x040000, 0x680feb3c, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdrabd)
STD_ROM_FN(Ssrdrabd)

static struct BurnRomInfo SsrdraddRomDesc[] = {
	{ "064add02.8e",        0x040000, 0x06b0138e, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064add03.8g",        0x040000, 0x03eb8b91, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdradd)
STD_ROM_FN(Ssrdradd)

static struct BurnRomInfo SsrdrjbdRomDesc[] = {
	{ "064jbd02.8e",        0x040000, 0x7acdc1e3, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "064jbd03.8g",        0x040000, 0x6a424918, BRF_ESS | BRF_PRG }, //  1
	{ "064eab04.10e",       0x020000, 0xef2315bd, BRF_ESS | BRF_PRG }, //  2
	{ "064eab05.10g",       0x020000, 0x51d6fbc4, BRF_ESS | BRF_PRG }, //  3
	
	{ "064e01.2f",          0x010000, 0x44b9bc52, BRF_ESS | BRF_PRG }, //  4	Z80 Program 
	
	{ "064e12.16k",         0x080000, 0xe2bdc619, BRF_GRA },	   //  5	Tiles
	{ "064e11.12k",         0x080000, 0x2d8ca8b0, BRF_GRA },	   //  6
	
	{ "064e09.7l",          0x100000, 0x4160c372, BRF_GRA },	   //  7	Sprites
	{ "064e07.3l",          0x100000, 0x64dd673c, BRF_GRA },	   //  8
	
	{ "064e06.1d",          0x100000, 0x59810df9, BRF_SND },	   //  9	K053260 Samples
};

STD_ROM_PICK(Ssrdrjbd)
STD_ROM_FN(Ssrdrjbd)

static struct BurnRomInfo sunsetblRomDesc[] = {
	{ "sunsetb.03",		0x080000, 0x37ffe90b, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "sunsetb.04",		0x080000, 0x8ff647b7, BRF_ESS | BRF_PRG }, //  1

	{ "064e12.16k",		0x080000, 0xe2bdc619, BRF_GRA },	   //  2	Tiles
	{ "064e11.12k",		0x080000, 0x2d8ca8b0, BRF_GRA },	   //  3

	{ "sunsetb.05",		0x080000, 0x8a0ff31a, BRF_GRA },	   //  4	Sprites
	{ "sunsetb.06",		0x080000, 0xfdf2c887, BRF_GRA },	   //  5
	{ "sunsetb.07",		0x080000, 0xa545b1ed, BRF_GRA },	   //  6
	{ "sunsetb.08",		0x080000, 0xf867cd38, BRF_GRA },	   //  7

	{ "sunsetb.01",		0x080000, 0x1a8b5ca2, BRF_SND },	   //  8	OKI Samples
	{ "sunsetb.02",		0x080000, 0x5d485523, BRF_SND },	   //  9
};

STD_ROM_PICK(sunsetbl)
STD_ROM_FN(sunsetbl)

static struct BurnRomInfo thndrx2RomDesc[] = {
	{ "073-ea-l02.11c",	0x020000, 0xeae02b51, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "073-ea-l03.12c",	0x020000, 0x738ed007, BRF_ESS | BRF_PRG }, //  1

	{ "073-c01.4f",		0x010000, 0x44ebe83c, BRF_ESS | BRF_PRG }, //  2 	Z80 Code

	{ "073-c06.16k",	0x080000, 0x24e22b42, BRF_GRA },           //  3 	Tiles
	{ "073-c05.12k",	0x080000, 0x952a935f, BRF_GRA },           //  4

	{ "073-c07.7k",		0x080000, 0x14e93f38, BRF_GRA },           //  5 	Sprites
	{ "073-c08.3k",		0x080000, 0x09fab3ab, BRF_GRA },           //  6

	{ "073-b04.2d",		0x080000, 0x05287a0b, BRF_SND },           //  7 	K053260 Samples
};

STD_ROM_PICK(thndrx2)
STD_ROM_FN(thndrx2)

static struct BurnRomInfo thndrx2aRomDesc[] = {
	{ "073-aa-m02.11c",	0x020000, 0x5b5b4cc0, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "073-aa-m03.12c",	0x020000, 0x320435a8, BRF_ESS | BRF_PRG }, //  1

	{ "073-c01.4f",		0x010000, 0x44ebe83c, BRF_ESS | BRF_PRG }, //  2 	Z80 Code

	{ "073-c06.16k",	0x080000, 0x24e22b42, BRF_GRA },           //  3 	Tiles
	{ "073-c05.12k",	0x080000, 0x952a935f, BRF_GRA },           //  4

	{ "073-c07.7k",		0x080000, 0x14e93f38, BRF_GRA },           //  5 	Sprites
	{ "073-c08.3k",		0x080000, 0x09fab3ab, BRF_GRA },           //  6

	{ "073-b04.2d",		0x080000, 0x05287a0b, BRF_SND },           //  7 	K053260 Samples
};

STD_ROM_PICK(thndrx2a)
STD_ROM_FN(thndrx2a)

static struct BurnRomInfo thndrx2jRomDesc[] = {
	{ "073-ja-k02.11c",	0x020000, 0x0c8b2d3f, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "073-ja-k03.12c",	0x020000, 0x3803b427, BRF_ESS | BRF_PRG }, //  1

	{ "073-c01.4f",		0x010000, 0x44ebe83c, BRF_ESS | BRF_PRG }, //  2 	Z80 Code

	{ "073-c06.16k",	0x080000, 0x24e22b42, BRF_GRA },           //  3 	Tiles
	{ "073-c05.12k",	0x080000, 0x952a935f, BRF_GRA },           //  4

	{ "073-c07.7k",		0x080000, 0x14e93f38, BRF_GRA },           //  5 	Sprites
	{ "073-c08.3k",		0x080000, 0x09fab3ab, BRF_GRA },           //  6

	{ "073-b04.2d",		0x080000, 0x05287a0b, BRF_SND },           //  7 	K053260 Samples
};

STD_ROM_PICK(thndrx2j)
STD_ROM_FN(thndrx2j)

static struct BurnRomInfo lgtnfghtRomDesc[] = {
	{ "939k02.e11",		0x020000, 0x2dfefa53, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "939k03.e15",		0x020000, 0x14f0c454, BRF_ESS | BRF_PRG }, //  1

	{ "939e01.d7",		0x008000, 0x4a5fc848, BRF_ESS | BRF_PRG }, //  2 	Z80 Code

	{ "939a07.k14",		0x080000, 0x7955dfcf, BRF_GRA },           //  3 	Tiles
	{ "939a08.k19",		0x080000, 0xed95b385, BRF_GRA },           //  4

	{ "939a06.k8",		0x080000, 0xe393c206, BRF_GRA },           //  5 	Sprites
	{ "939a05.k2",		0x080000, 0x3662d47a, BRF_GRA },           //  6

	{ "939a04.c5",		0x080000, 0xc24e2b6e, BRF_SND },           //  7 	K053260 Samples
};

STD_ROM_PICK(lgtnfght)
STD_ROM_FN(lgtnfght)

static struct BurnRomInfo lgtnfghuRomDesc[] = {
	{ "939m02.e11",		0x020000, 0x61a12184, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "939m03.e15",		0x020000, 0x6db6659d, BRF_ESS | BRF_PRG }, //  1

	{ "939e01.d7",		0x008000, 0x4a5fc848, BRF_ESS | BRF_PRG }, //  2 	Z80 Code

	{ "939a07.k14",		0x080000, 0x7955dfcf, BRF_GRA },           //  3 	Tiles
	{ "939a08.k19",		0x080000, 0xed95b385, BRF_GRA },           //  4

	{ "939a06.k8",		0x080000, 0xe393c206, BRF_GRA },           //  5 	Sprites
	{ "939a05.k2",		0x080000, 0x3662d47a, BRF_GRA },           //  6

	{ "939a04.c5",		0x080000, 0xc24e2b6e, BRF_SND },           //  7 	K053260 Samples
};

STD_ROM_PICK(lgtnfghu)
STD_ROM_FN(lgtnfghu)

static struct BurnRomInfo lgtnfghaRomDesc[] = {
	{ "939j_02.e11",	0x020000, 0xbbbb4a74, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "939j_03.e15",	0x020000, 0x8d4da7b7, BRF_ESS | BRF_PRG }, //  1

	{ "939e01.d7",		0x008000, 0x4a5fc848, BRF_ESS | BRF_PRG }, //  2 	Z80 Code

	{ "939a07.k14",		0x080000, 0x7955dfcf, BRF_GRA },           //  3 	Tiles
	{ "939a08.k19",		0x080000, 0xed95b385, BRF_GRA },           //  4

	{ "939a06.k8",		0x080000, 0xe393c206, BRF_GRA },           //  5 	Sprites
	{ "939a05.k2",		0x080000, 0x3662d47a, BRF_GRA },           //  6

	{ "939a04.c5",		0x080000, 0xc24e2b6e, BRF_SND },           //  7 	K053260 Samples
};

STD_ROM_PICK(lgtnfgha)
STD_ROM_FN(lgtnfgha)

static struct BurnRomInfo trigonRomDesc[] = {
	{ "939j02.e11",		0x020000, 0x38381d1b, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "939j03.e15",		0x020000, 0xb5beddcd, BRF_ESS | BRF_PRG }, //  1

	{ "939e01.d7",		0x008000, 0x4a5fc848, BRF_ESS | BRF_PRG }, //  2 	Z80 Code

	{ "939a07.k14",		0x080000, 0x7955dfcf, BRF_GRA },           //  3 	Tiles
	{ "939a08.k19",		0x080000, 0xed95b385, BRF_GRA },           //  4

	{ "939a06.k8",		0x080000, 0xe393c206, BRF_GRA },           //  5 	Sprites
	{ "939a05.k2",		0x080000, 0x3662d47a, BRF_GRA },           //  6

	{ "939a04.c5",		0x080000, 0xc24e2b6e, BRF_SND },           //  7 	K053260 Samples
};

STD_ROM_PICK(trigon)
STD_ROM_FN(trigon)

static struct BurnRomInfo tmnt2RomDesc[] = {
	{ "063uaa02.e8",	0x020000, 0x58d5c93d, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "063uaa03.g8",	0x020000, 0x0541fec9, BRF_ESS | BRF_PRG }, //  1
	{ "063uaa04.e10",	0x020000, 0x1d441a7d, BRF_ESS | BRF_PRG }, //  2
	{ "063uaa05.g10",	0x020000, 0x9c428273, BRF_ESS | BRF_PRG }, //  3

	{ "063b01",		0x010000, 0x364f548a, BRF_ESS | BRF_PRG }, //  4 	Z80 Code

	{ "063b12",		0x080000, 0xd3283d19, BRF_GRA },           //  5	Tiles
	{ "063b11",		0x080000, 0x6ebc0c15, BRF_GRA },           //  6

	{ "063b09",		0x100000, 0x2d7a9d2a, BRF_GRA },           //  7	Sprites
	{ "063b10",		0x080000, 0xf2dd296e, BRF_GRA },           //  8
	{ "063b07",		0x100000, 0xd9bee7bf, BRF_GRA },           //  9
	{ "063b08",		0x080000, 0x3b1ae36f, BRF_GRA },           // 10

	{ "063b06",		0x200000, 0x1e510aa5, BRF_SND },           // 11	K053260 Samples
};

STD_ROM_PICK(tmnt2)
STD_ROM_FN(tmnt2)

static struct BurnRomInfo tmnt22puRomDesc[] = {
	{ "063uda02.e8",	0x020000, 0xaadffe3a, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "063uda03.g8",	0x020000, 0x125687a8, BRF_ESS | BRF_PRG }, //  1
	{ "063uda04.e10",	0x020000, 0xfb5c7ded, BRF_ESS | BRF_PRG }, //  2
	{ "063uda05.g10",	0x020000, 0x3c40fe66, BRF_ESS | BRF_PRG }, //  3

	{ "063b01",		0x010000, 0x364f548a, BRF_ESS | BRF_PRG }, //  4 	Z80 Code

	{ "063b12",		0x080000, 0xd3283d19, BRF_GRA },           //  5	Tiles
	{ "063b11",		0x080000, 0x6ebc0c15, BRF_GRA },           //  6

	{ "063b09",		0x100000, 0x2d7a9d2a, BRF_GRA },           //  7	Sprites
	{ "063b10",		0x080000, 0xf2dd296e, BRF_GRA },           //  8
	{ "063b07",		0x100000, 0xd9bee7bf, BRF_GRA },           //  9
	{ "063b08",		0x080000, 0x3b1ae36f, BRF_GRA },           // 10

	{ "063b06",		0x200000, 0x1e510aa5, BRF_SND },           // 11	K053260 Samples
};

STD_ROM_PICK(tmnt22pu)
STD_ROM_FN(tmnt22pu)

static struct BurnRomInfo tmht22peRomDesc[] = {
	{ "063eba02.e8",	0x020000, 0x99409094, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "063eba03.g8",	0x020000, 0x4d65650f, BRF_ESS | BRF_PRG }, //  1
	{ "063eba04.e10",	0x020000, 0xf6e3b9c7, BRF_ESS | BRF_PRG }, //  2
	{ "063eba05.g10",	0x020000, 0x1bad6696, BRF_ESS | BRF_PRG }, //  3

	{ "063b01",		0x010000, 0x364f548a, BRF_ESS | BRF_PRG }, //  4 	Z80 Code

	{ "063b12",		0x080000, 0xd3283d19, BRF_GRA },           //  5	Tiles
	{ "063b11",		0x080000, 0x6ebc0c15, BRF_GRA },           //  6

	{ "063b09",		0x100000, 0x2d7a9d2a, BRF_GRA },           //  7	Sprites
	{ "063b10",		0x080000, 0xf2dd296e, BRF_GRA },           //  8
	{ "063b07",		0x100000, 0xd9bee7bf, BRF_GRA },           //  9
	{ "063b08",		0x080000, 0x3b1ae36f, BRF_GRA },           // 10

	{ "063b06",		0x200000, 0x1e510aa5, BRF_SND },           // 11	K053260 Samples
};

STD_ROM_PICK(tmht22pe)
STD_ROM_FN(tmht22pe)

static struct BurnRomInfo tmnt2aRomDesc[] = {
	{ "063ada02.e8",	0x020000, 0x4f11b587, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "063ada03.g8",	0x020000, 0x82a1b9ac, BRF_ESS | BRF_PRG }, //  1
	{ "063ada04.e10",	0x020000, 0x05ad187a, BRF_ESS | BRF_PRG }, //  2
	{ "063ada05.g10",	0x020000, 0xd4826547, BRF_ESS | BRF_PRG }, //  3

	{ "063b01",		0x010000, 0x364f548a, BRF_ESS | BRF_PRG }, //  4 	Z80 Code

	{ "063b12",		0x080000, 0xd3283d19, BRF_GRA },           //  5	Tiles
	{ "063b11",		0x080000, 0x6ebc0c15, BRF_GRA },           //  6

	{ "063b09",		0x100000, 0x2d7a9d2a, BRF_GRA },           //  7	Sprites
	{ "063b10",		0x080000, 0xf2dd296e, BRF_GRA },           //  8
	{ "063b07",		0x100000, 0xd9bee7bf, BRF_GRA },           //  9
	{ "063b08",		0x080000, 0x3b1ae36f, BRF_GRA },           // 10

	{ "063b06",		0x200000, 0x1e510aa5, BRF_SND },           // 11	K053260 Samples
};

STD_ROM_PICK(tmnt2a)
STD_ROM_FN(tmnt2a)

static struct BurnRomInfo qgakumonRomDesc[] = {
	{ "248jaa02.8e",	0x040000, 0xfab79410, BRF_ESS | BRF_PRG }, //  0 	68000 Program Code
	{ "248jaa03.8g",	0x040000, 0x8d888ef3, BRF_ESS | BRF_PRG }, //  1
	{ "248jaa04.10e",	0x040000, 0x56cb16cb, BRF_ESS | BRF_PRG }, //  2
	{ "248jaa05.10g",	0x040000, 0x27614fcd, BRF_ESS | BRF_PRG }, //  3

	{ "248a01.2f",		0x010000, 0xa8a41cc6, BRF_ESS | BRF_PRG }, //  4 	Z80 Code

	{ "248a12.16k",		0x080000, 0x62870987, BRF_GRA },           //  5 	Tiles
	{ "248a11.12k",		0x080000, 0xfad2dbfd, BRF_GRA },           //  6

	{ "248a09.7l",		0x100000, 0xa176e205, BRF_GRA },           //  7	Sprites
	{ "248a07.3l",		0x100000, 0x9595589f, BRF_GRA },           //  8

	{ "248a06.1d",		0x200000, 0x0fba1def, BRF_SND },           //  9	K053260 Samples
};

STD_ROM_PICK(qgakumon)
STD_ROM_FN(qgakumon)

static struct BurnRomInfo punkshotRomDesc[] = {
	{ "907-j02.i7",		0x020000, 0xdbb3a23b, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "907-j03.i10",	0x020000, 0x2151d1ab, BRF_ESS | BRF_PRG }, //  1

	{ "907f01.e8",		0x008000, 0xf040c484, BRF_ESS | BRF_PRG }, //  2	Z80 Program

	{ "907d06.e23",		0x040000, 0xf5cc38f4, BRF_GRA },	   //  3	Tiles
	{ "907d05.e22",		0x040000, 0xe25774c1, BRF_GRA },	   //  4

	{ "907d07.k2",		0x100000, 0xb0fe4543, BRF_GRA },	   //  5	Sprites
	{ "907d08.k7",		0x100000, 0xd5ac8d9d, BRF_GRA },	   //  6

	{ "907d04.d3",		0x080000, 0x090feb5e, BRF_SND },	   //  7	K053260 Samples
};

STD_ROM_PICK(punkshot)
STD_ROM_FN(punkshot)

static struct BurnRomInfo punksht2RomDesc[] = {
	{ "907m02.i7",		0x020000, 0x59e14575, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "907m03.i10",		0x020000, 0xadb14b1e, BRF_ESS | BRF_PRG }, //  1

	{ "907f01.e8",		0x008000, 0xf040c484, BRF_ESS | BRF_PRG }, //  2	Z80 Program

	{ "907d06.e23",		0x040000, 0xf5cc38f4, BRF_GRA },	   //  3	Tiles
	{ "907d05.e22",		0x040000, 0xe25774c1, BRF_GRA },	   //  4

	{ "907d07.k2",		0x100000, 0xb0fe4543, BRF_GRA },	   //  5	Sprites
	{ "907d08.k7",		0x100000, 0xd5ac8d9d, BRF_GRA },	   //  6

	{ "907d04.d3",		0x080000, 0x090feb5e, BRF_SND },	   //  7	K053260 Samples
};

STD_ROM_PICK(punksht2)
STD_ROM_FN(punksht2)

static struct BurnRomInfo punkshtjRomDesc[] = {
	{ "907z02.i7",		0x020000, 0x7a3a5c89, BRF_ESS | BRF_PRG }, //  0	68000 Program Code
	{ "907z03.i10",		0x020000, 0x22a3d9d6, BRF_ESS | BRF_PRG }, //  1

	{ "907f01.e8",		0x008000, 0xf040c484, BRF_ESS | BRF_PRG }, //  2	Z80 Program

	{ "907d06.e23",		0x040000, 0xf5cc38f4, BRF_GRA },	   //  3	Tiles
	{ "907d05.e22",		0x040000, 0xe25774c1, BRF_GRA },	   //  4

	{ "907d07.k2",		0x100000, 0xb0fe4543, BRF_GRA },	   //  5	Sprites
	{ "907d08.k7",		0x100000, 0xd5ac8d9d, BRF_GRA },	   //  6

	{ "907d04.d3",		0x080000, 0x090feb5e, BRF_SND },	   //  7	K053260 Samples
};

STD_ROM_PICK(punkshtj)
STD_ROM_FN(punkshtj)

static int TmntMemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x060000;
	DrvZ80Rom              = Next; Next += 0x008000;
	DrvSoundRom            = Next; Next += 0x020000;
	DrvUPD7759CRom         = Next; Next += 0x020000;
	DrvTileRom             = Next; Next += 0x100000;
	DrvSpriteRom           = Next; Next += 0x200000;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x004000;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;

	RamEnd                 = Next;

	Palette                = (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);
	DrvTitleSample         = (INT16*)Next; Next += 0x40000 * sizeof(INT16);
	DrvTiles               = Next; Next += 0x008000 * 8 * 8;
	DrvSprites             = Next; Next += 0x004000 * 16 * 16;

	MemEnd                 = Next;

	return 0;
}

static int MiaMemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x040000;
	DrvZ80Rom              = Next; Next += 0x008000;
	DrvSoundRom            = Next; Next += 0x020000;
	DrvTileRom             = Next; Next += 0x040000;
	DrvSpriteRom           = Next; Next += 0x100000;
		
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x008000;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;

	RamEnd                 = Next;

	Palette                = (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);
	DrvTiles               = Next; Next += 0x002000 * 8 * 8;
	DrvSprites             = Next; Next += 0x002000 * 16 * 16;

	MemEnd                 = Next;

	return 0;
}

static int CuebrickMemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x020000;
	DrvTileRom             = Next; Next += 0x040000;
	DrvSpriteRom           = Next; Next += 0x040000;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x008000;
	DrvPaletteRam          = Next; Next += 0x001000;
	
	RamEnd                 = Next;

	Palette                = (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);
	DrvTiles               = Next; Next += 0x002000 * 8 * 8;
	DrvSprites             = Next; Next += 0x001000 * 16 * 16;
	DrvNvRam               = Next; Next += 0x400 * 0x20;

	MemEnd                 = Next;

	return 0;
}

static int BlswhstlMemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x080000;
	DrvZ80Rom              = Next; Next += 0x010000;
	DrvSoundRom            = Next; Next += 0x100000;
	DrvTileRom             = Next; Next += 0x100000;
	DrvSpriteRom           = Next; Next += 0x100000;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x004000;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;
	DrvSpriteRam           = Next; Next += 0x004000;

	RamEnd                 = Next;
	
	DrvTiles               = Next; Next += 0x008000 * 8 * 8;
	DrvSprites             = Next; Next += 0x002000 * 16 * 16;
	Palette                = (unsigned int*)Next; Next += 0x00800 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00810 * sizeof(unsigned int);

	MemEnd                 = Next;

	return 0;
}

static int SsridersMemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x0c0000;
	DrvZ80Rom              = Next; Next += 0x010000;
	DrvSoundRom            = Next; Next += 0x100000;
	DrvTileRom             = Next; Next += 0x100000;
	DrvSpriteRom           = Next; Next += 0x200000;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x004080;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;
	DrvSpriteRam           = Next; Next += 0x004000;

	RamEnd                 = Next;

	Palette                = (unsigned int*)Next; Next += 0x00800 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00810 * sizeof(unsigned int);
	DrvTiles               = Next; Next += 0x008000 * 8 * 8;
	DrvSprites             = Next; Next += 0x004000 * 16 * 16;

	MemEnd                 = Next;

	return 0;
}

static int Tmnt2MemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x100000;
	DrvZ80Rom              = Next; Next += 0x010000;
	DrvSoundRom            = Next; Next += 0x200000;
	DrvTileRom             = Next; Next += 0x100000;
	DrvSpriteRom           = Next; Next += 0x400000;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x005080;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;
	DrvSpriteRam           = Next; Next += 0x004000;

	RamEnd                 = Next;

	Palette                = (unsigned int*)Next; Next += 0x00800 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00810 * sizeof(unsigned int);
	DrvTiles               = Next; Next += 0x008000 * 8 * 8;
	DrvSprites             = Next; Next += 0x008000 * 16 * 16;

	MemEnd                 = Next;

	return 0;
}

static int Thndrx2MemIndex()
{
	unsigned char *Next; Next = Mem;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x004000;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;

	RamEnd                 = Next;

	Drv68KRom              = Next; Next += 0x080000;
	DrvZ80Rom              = Next; Next += 0x010000;
	DrvSoundRom            = Next; Next += 0x080000;
	DrvTileRom             = Next; Next += 0x100000;
	DrvSpriteRom           = Next; Next += 0x100000;
	
	Palette                = (unsigned int*)Next; Next += 0x00800 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00810 * sizeof(unsigned int);
	DrvTiles               = Next; Next += 0x008000 * 8 * 8;
	DrvSprites             = Next; Next += 0x002000 * 16 * 16;

	MemEnd                 = Next;

	return 0;
}

static int LgtnfghtMemIndex()
{
	unsigned char *Next; Next = Mem;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x004000;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;
	DrvSpriteRam           = Next; Next += 0x004000;

	RamEnd                 = Next;

	Drv68KRom              = Next; Next += 0x080000;
	DrvZ80Rom              = Next; Next += 0x010000;
	DrvSoundRom            = Next; Next += 0x080000;
	DrvTileRom             = Next; Next += 0x100000;
	DrvSpriteRom           = Next; Next += 0x100000;

	Palette                = (unsigned int*)Next; Next += 0x00800 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00810 * sizeof(unsigned int);
	DrvTiles               = Next; Next += 0x008000 * 8 * 8;
	DrvSprites             = Next; Next += 0x002000 * 16 * 16;

	MemEnd                 = Next;

	return 0;
}

static int PunkshotMemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68KRom              = Next; Next += 0x040000;
	DrvZ80Rom              = Next; Next += 0x010000;
	DrvSoundRom            = Next; Next += 0x080000;
	DrvTileRom             = Next; Next += 0x080000;
	DrvSpriteRom           = Next; Next += 0x200000;
	
	RamStart               = Next;

	Drv68KRam              = Next; Next += 0x004000;
	DrvZ80Ram              = Next; Next += 0x000800;
	DrvPaletteRam          = Next; Next += 0x001000;

	RamEnd                 = Next;

	Palette                = (unsigned int*)Next; Next += 0x00800 * sizeof(unsigned int);
	DrvPalette             = (unsigned int*)Next; Next += 0x00810 * sizeof(unsigned int);
	DrvTiles               = Next; Next += 0x004000 * 8 * 8;
	DrvSprites             = Next; Next += 0x004000 * 16 * 16;

	MemEnd                 = Next;

	return 0;
}

static int DrvDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();
	
	ZetOpen(0);
	ZetReset();
	ZetClose();
	
	BurnYM2151Reset();
	
	KonamiICReset();
	
	bIrqEnable = 0;
	DrvSoundLatch = 0;
	TitleSoundLatch = 0;
	PlayTitleSample = 0;
	TitleSamplePos = 0;
	PriorityFlag = 0;
	
	return 0;
}

static int TmntDoReset()
{
	int nRet = DrvDoReset();
	
	UPD7759Reset();
	
	UPD7759StartWrite(0, 0);
	UPD7759ResetWrite(0, 1);
	
	return nRet;
}

static int CuebrickDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();
	
	BurnYM2151Reset();	
	
	KonamiICReset();
	
	bIrqEnable = 0;
	DrvNvRamBank = 0;
	PriorityFlag = 0;
	CuebrickSndIrqFire = 0;
	
	return 0;
}

static int BlswhstlDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();
	
	ZetOpen(0);
	ZetReset();
	ZetClose();
	
	BurnYM2151Reset();
	K053260Reset(0);
	
	KonamiICReset();
	
	EEPROMReset();
	
	if (EEPROMAvailable()) {
		InitEEPROMCount = 0;
	} else {
		InitEEPROMCount = 10;
	}
	
	BlswhstlTileRomBank = 0;
	
	K052109_irq_enabled = 0;
	
	return 0;
}

static int SsridersDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();
	
	ZetOpen(0);
	ZetReset();
	ZetClose();
	
	BurnYM2151Reset();
	K053260Reset(0);
	
	KonamiICReset();
	
	EEPROMReset();

	if (EEPROMAvailable()) {
		InitEEPROMCount = 0;
	} else {
		InitEEPROMCount = 10;
	}
	
	K052109_irq_enabled = 0;
	DrvVBlank = 0;

	dim_c = 0;
	dim_v = 0;

	return 0;
}

static int PunkshotDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();
	
	ZetOpen(0);
	ZetReset();
	ZetClose();
	
	BurnYM2151Reset();
	K053260Reset(0);
	
	KonamiICReset();
	
	K052109_irq_enabled = 0;
	DrvVBlank = 0;

	return 0;
}

unsigned char __fastcall Tmnt68KReadByte(unsigned int a)
{
	K052109WordNoA12Read(0x100000)
	K051937ByteRead(0x140000)
	K051960ByteRead(0x140400)
	
	switch (a) {
		case 0x0a0001: {
			return 0xff - DrvInput[0];
		}
		
		case 0x0a0003: {
			return 0xff - DrvInput[1];
		}
		
		case 0x0a0005: {
			return 0xff - DrvInput[2];
		}
		
		case 0x0a0007: {
			return 0xff - DrvInput[3];
		}
		
		case 0x0a0011: {
			return DrvDip[0];
		}
		
		case 0x0a0013: {
			return DrvDip[1];
		}
		
		case 0x0a0015: {
			return 0xff - DrvInput[4];
		}
		
		case 0x0a0019: {
			return DrvDip[2];
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read byte => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Tmnt68KWriteByte(unsigned int a, unsigned char d)
{
	K052109WordNoA12Write(0x100000)
	K015937ByteWrite(0x140000)
	K051960ByteWrite(0x140400)

	switch (a) {
		case 0x0a0001: {
			static int Last;
			if (Last == 0x08 && (d & 0x08) == 0) {
				ZetOpen(0);
				ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
				ZetClose();
			}
			Last = d & 0x08;
			
			bIrqEnable = (d & 0x20) ? 1 : 0;
			
			K052109RMRDLine = d & 0x80;
			return;
		}
		
		case 0x0a0009: {
			DrvSoundLatch = d;
			return;
		}
		
		case 0x0a0011: {
			// watchdog write
			return;
		}
		
		case 0x0c0001: {
			PriorityFlag = (d & 0x0c) >> 2;
			return;
		}
		
		case 0x10e801: {
			// nop???
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write byte => %06X, %02X\n"), a, d);
		}
	}
}

unsigned short __fastcall Tmnt68KReadWord(unsigned int a)
{
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read word => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Tmnt68KWriteWord(unsigned int a, unsigned short d)
{
	K051960WordWrite(0x140400)
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write word => %06X, %04X\n"), a, d);
		}
	}
}

unsigned char __fastcall Mia68KReadByte(unsigned int a)
{
	K052109WordNoA12Read(0x100000)
	K051937ByteRead(0x140000)
	K051960ByteRead(0x140400)
	
	switch (a) {
		case 0x0a0001: {
			return 0xff - DrvInput[0];
		}
		
		case 0x0a0003: {
			return 0xff - DrvInput[1];
		}
		
		case 0x0a0005: {
			return 0xff - DrvInput[2];
		}
		
		case 0x0a0011: {
			return DrvDip[0];
		}
		
		case 0x0a0013: {
			return DrvDip[1];
		}
		
		case 0x0a0019: {
			return DrvDip[2];
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read byte => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Mia68KWriteByte(unsigned int a, unsigned char d)
{
	K052109WordNoA12Write(0x100000)
	K015937ByteWrite(0x140000)
	K051960ByteWrite(0x140400)
	
	switch (a) {
		case 0x0a0001: {
			static int Last;
			if (Last == 0x08 && (d & 0x08) == 0) {
				ZetOpen(0);
				ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
				ZetClose();
			}
			Last = d & 0x08;
			
			bIrqEnable = (d & 0x20) ? 1 : 0;
			
			K052109RMRDLine = d & 0x80;
			return;
		}
		
		case 0x0a0009: {
			DrvSoundLatch = d;
			return;
		}
		
		case 0x0a0011: {
			// watchdog write
			return;
		}
		
		case 0x10e801: {
			// nop???
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write byte => %06X, %02X\n"), a, d);
		}
	}
}

unsigned short __fastcall Mia68KReadWord(unsigned int a)
{
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read word => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Mia68KWriteWord(unsigned int a, unsigned short d)
{
	K051960WordWrite(0x140400)
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write word => %06X, %04X\n"), a, d);
		}
	}
}

unsigned char __fastcall Cuebrick68KReadByte(unsigned int a)
{
	K052109WordNoA12Read(0x100000)
	K051937ByteRead(0x140000)
	K051960ByteRead(0x140400)
	
	if (a >= 0x0b0000 && a <= 0x0b03ff) {
		return DrvNvRam[(a - 0x0b0000) + (DrvNvRamBank * 0x400)];
	}
	
	switch (a) {
		case 0x0a0001: {
			return 0xff - DrvInput[0];
		}
		
		case 0x0a0003: {
			return 0xff - DrvInput[1];
		}
		
		case 0x0a0005: {
			return 0xff - DrvInput[2];
		}
		
		case 0x0a0011: {
			return DrvDip[1];
		}
		
		case 0x0a0013: {
			return DrvDip[0];
		}
		
		case 0x0a0019: {
			return DrvDip[2];
		}
		
		case 0x0c0000:
			return 0xff;
			
		case 0x0c0002: {
			return BurnYM2151ReadStatus();
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read byte => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Cuebrick68KWriteByte(unsigned int a, unsigned char d)
{
	K052109WordNoA12Write(0x100000)
	K015937ByteWrite(0x140000)
	K051960ByteWrite(0x140400)
	
	if (a >= 0x0b0000 && a <= 0x0b03ff) {
		DrvNvRam[(a - 0x0b0000) + (DrvNvRamBank * 0x400)] = d;
		return;
	}
	
	switch (a) {
		case 0x0a0001: {
			bIrqEnable = (d & 0x20) ? 1 : 0;
			
			K052109RMRDLine = d & 0x80;
			return;
		}
		
		case 0x0a0011: {
			// watchdog write
			return;
		}
		
		case 0x0b0400: {
			DrvNvRamBank = d;
			return;
		}
		
		case 0x0c0000: {
			BurnYM2151SelectRegister(d);
			return;
		}
		
		case 0x0c0002: {
			BurnYM2151WriteRegister(d);
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write byte => %06X, %02X\n"), a, d);
		}
	}
}

unsigned short __fastcall Cuebrick68KReadWord(unsigned int a)
{
	if (a >= 0x0b0000 && a <= 0x0b03ff) {
		SEK_DEF_READ_WORD(0, a);
		return 0;
	}
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read word => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Cuebrick68KWriteWord(unsigned int a, unsigned short d)
{
	K051960WordWrite(0x140400)
	
	if (a >= 0x0b0000 && a <= 0x0b03ff) {
		SEK_DEF_WRITE_WORD(0, a, d);
		return;
	}
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write word => %06X, %04X\n"), a, d);
		}
	}
}

unsigned char __fastcall Blswhstl68KReadByte(unsigned int a)
{
	if (a >= 0x180000 && a <= 0x183fff) {
		int Offset = (a - 0x180000) >> 1;
		
		if (a & 1) {
			return K052109Read(Offset + 0x2000);
		} else {
			return K052109Read(Offset + 0x0000);
		}
	}
	
	if (a >= 0x300000 && a <= 0x303fff) {
		int Offset = (a - 0x300000);
		
		if (Offset & (0x31 << 1)) {
			return DrvSpriteRam[Offset ^ 1];
		} else {
			Offset = ((Offset & (0x000e << 1)) >> 1) | ((Offset & (0x1fc0 << 1)) >> 3);
			return K053245ReadWord(0, Offset/2) >> ((~a & 1) << 3);
		}
	}
	
	if (a >= 0x500000 && a <= 0x50003f) {
		int Offset = (a - 0x500000) >> 1;
		if (a & 1) {
			return K054000Read(Offset);
		} else {
			return 0;
		}
	}
	
	if (a >= 0x680000 && a <= 0x68001f) {
		int Offset = (a - 0x680000) >> 1;
		Offset &= ~1;
		
		if (a & 1) {
			return K053244Read(0, Offset + 1);
		} else {
			return K053244Read(0, Offset + 0);
		}
	}
	
	switch (a) {
		case 0x700001: {
			return 0xff - DrvInput[1];
		}
		
		case 0x700003: {
			return 0xff - DrvInput[2];
		}
		
		case 0x700005: {
			unsigned char Res;
			static int Toggle;

			Res = 0xff - DrvInput[0];
			
			if (InitEEPROMCount) {
				InitEEPROMCount--;
				Res &= 0xf7;
			}
			
			Toggle ^= 0x40;
			
			return Res ^ Toggle;
		}
		
		case 0x700007: {
			return 0xfe | (EEPROMRead() & 0x01);
		}
		
		case 0x780601:
		case 0x780603: {
			int Offset = (a - 0x780601) >> 1;
			return K053260Read(0, Offset + 2);
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read byte => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Blswhstl68KWriteByte(unsigned int a, unsigned char d)
{
	if (a >= 0x180000 && a <= 0x183fff) {
		int Offset = (a - 0x180000) >> 1;
		
		if (a & 1) {
			K052109Write(Offset + 0x2000, d);
		} else {
			K052109Write(Offset + 0x0000, d);
		}
		return;
	}

	if (a >= 0x300000 && a <= 0x303fff) {
		int Offset = (a - 0x300000);
		DrvSpriteRam[Offset ^ 1] = d;

		if (!(Offset & (0x31 << 1))) {
			Offset = ((Offset & (0x000e << 1)) >> 1) | ((Offset & (0x1fc0 << 1)) >> 3);

			int data = K053245ReadWord(0, Offset/2);

			if (a & 1) {
				data &= 0xff00;
				data |= d;
			} else {
				data &= 0x00ff;
				data |= d << 8;
			}

			K053245WriteWord(0, Offset / 2, data);
		}
		return;
	}

	if (a >= 0x500000 && a <= 0x50003f) {
		int Offset = (a - 0x500000) >> 1;
		if (a & 1) K054000Write(Offset, d);
		return;
	}
	
	if (a >= 0x680000 && a <= 0x68001f) {
		int Offset = (a - 0x680000) >> 1;
		Offset &= ~1;
		
		if (a & 1) {
			K053244Write(0, Offset + 1, d);
		} else {
			K053244Write(0, Offset + 0, d);
		}
		return;
	}
	
	if (a >= 0x780700 && a <= 0x78071f) {
		K053251Write((a - 0x780700) >> 1, d);
		return;
	}
	
	switch (a) {
		case 0x700201: {
			EEPROMWrite(d & 0x04, d & 0x02, d & 0x01);
			return;
		}
		
		case 0x700301: {
			K052109RMRDLine = d & 0x08;
			BlswhstlTileRomBank = (d & 0x80) >> 7;
			return;
		}
		
		case 0x700401: {
			// watchdog write
			return;
		}
		
		case 0x780601: {
			K053260Write(0, 0, d);
			return;
		}
		
		case 0x780605: {
			ZetOpen(0);
			ZetSetVector(0xff);
			ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
			ZetClose();
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write byte => %06X, %02X\n"), a, d);
		}
	}
}

unsigned short __fastcall Blswhstl68KReadWord(unsigned int a)
{
	if (a >= 0x300000 && a <= 0x303fff) {
		UINT16 *SpriteRam = (UINT16*)DrvSpriteRam;
		int Offset = (a - 0x300000) >> 1;
		
		if (Offset & 0x31) {
			return SpriteRam[Offset];
		} else {
			Offset = ((Offset & 0x000e) >> 1) | ((Offset & 0x1fc0) >> 3);
			return K053245ReadWord(0, Offset);
		}
	}
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read word => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Blswhstl68KWriteWord(unsigned int a, unsigned short d)
{
	if (a >= 0x180000 && a <= 0x183fff) {
		int Offset = (a - 0x180000) >> 1;

		K052109Write(Offset + 0x2000, d >> 8);
		K052109Write(Offset + 0x0000, d & 0xff);
		return;
	}
	
	if (a >= 0x300000 && a <= 0x303fff) {
		UINT16 *SpriteRam = (UINT16*)DrvSpriteRam;
		int Offset = (a - 0x300000) >> 1;
		SpriteRam[Offset] = d;
		
		if (!(Offset & 0x0031)) {
			Offset = ((Offset & 0x000e) >> 1) | ((Offset & 0x1fc0) >> 3);
			K053245WriteWord(0, Offset, d);
		}
		return;
	}
	
	if (a >= 0x500000 && a <= 0x50003f) {
		int Offset = (a - 0x500000) >> 1;
		K054000Write(Offset, d & 0xff);
		return;
	}
	
	if (a >= 0x680000 && a <= 0x68001f) {
		int Offset = (a - 0x680000) >> 1;
		Offset &= ~1;
		
		K053244Write(0, Offset + 1, d & 0xff);
		K053244Write(0, Offset + 0, d >> 8);
		return;
	}
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write word => %06X, %04X\n"), a, d);
		}
	}
}

static void tmnt2_protection_write()
{
	UINT32 src_addr, dst_addr, mod_addr, attr1, code, attr2, cbase, cmod, color;
	int xoffs, yoffs, xmod, ymod, zmod, xzoom, yzoom, i;
	UINT16 *mcu;
	UINT16 src[4], mod[24];
	UINT8 keepaspect, xlock, ylock, zlock;

	mcu = (unsigned short*)(Drv68KRam + 0x4800);
	if ((mcu[8] & 0xff00) != 0x8200) return;

	src_addr = (mcu[0] | (mcu[1]&0xff)<<16) >> 1;
	dst_addr = (mcu[2] | (mcu[3]&0xff)<<16) >> 1;
	mod_addr = (mcu[4] | (mcu[5]&0xff)<<16) >> 1;
	zlock    = (mcu[8] & 0xff) == 0x0001;

	for (i=0; i< 4; i++) src[i] = SekReadWord((src_addr + i) * 2);
	for (i=0; i<24; i++) mod[i] = SekReadWord((mod_addr + i) * 2);

	code = src[0];			// code

	i= src[1];
	attr1 = i>>2 & 0x3f00;	// flip y, flip x and sprite size
	attr2 = i & 0x380;		// mirror y, mirror x, shadow
	cbase = i & 0x01f;		// base color
	cmod  = mod[0x2a/2]>>8;
	color = (cbase != 0x0f && cmod <= 0x1f && !zlock) ? cmod : cbase;

	xoffs = (INT16)src[2];	// local x
	yoffs = (INT16)src[3];	// local y

	i = mod[0];
	attr2 |= i & 0x0060;	// priority
	keepaspect = (i & 0x0014) == 0x0014;
	if (i & 0x8000) { attr1 |= 0x8000; }	// active
	if (keepaspect)	{ attr1 |= 0x4000; }	// keep aspect
	if (i & 0x4000) { attr1 ^= 0x1000; xoffs = -xoffs; }	// flip x

	xmod = (INT16)mod[6];	// global x
	ymod = (INT16)mod[7];	// global y
	zmod = (INT16)mod[8];	// global z
	xzoom = mod[0x1c/2];
	yzoom = (keepaspect) ? xzoom : mod[0x1e/2];

	ylock = xlock = (i & 0x0020 && (!xzoom || xzoom == 0x100));

	if (!xlock)
	{
		i = xzoom - 0x4f00;
		if (i > 0)
		{
			i >>= 8;
			xoffs += (int)(pow(i, /*1.898461*/1.891292) * xoffs / 599.250121);
		}
		else if (i < 0)
		{
			i = (i>>3) + (i>>4) + (i>>5) + (i>>6) + xzoom;
			xoffs = (i > 0) ? (xoffs * i / 0x4f00) : 0;
		}
	}
	if (!ylock)
	{
		i = yzoom - 0x4f00;
		if (i > 0)
		{
			i >>= 8;
			yoffs += (int)(pow(i, /*1.898461*/1.891292) * yoffs / 599.250121);
		}
		else if (i < 0)
		{
			i = (i>>3) + (i>>4) + (i>>5) + (i>>6) + yzoom;
			yoffs = (i > 0) ? (yoffs * i / 0x4f00) : 0;
		}

	}
	if (!zlock) yoffs += zmod;
	xoffs += xmod;
	yoffs += ymod;

	SekWriteWord((dst_addr +  0) * 2, attr1);
	SekWriteWord((dst_addr +  2) * 2, code);
	SekWriteWord((dst_addr +  4) * 2, (UINT32)yoffs);
	SekWriteWord((dst_addr +  6) * 2, (UINT32)xoffs);
	SekWriteWord((dst_addr + 12) * 2, attr2 | color);
}

void __fastcall Tmnt268KProtWord(unsigned int a, unsigned short d)
{
	*((unsigned short*)(Drv68KRam + 0x4800 + (a & 0x1e))) = d;
}

void __fastcall Tmnt268KProtByte(unsigned int a, unsigned char d)
{
	a &= 0x1f;
	Drv68KRam[0x4801 ^ a] = d;

	if (a == 0x18) tmnt2_protection_write();
}

static unsigned short ssriders_protection_r()
{
    int data = SekReadWord(0x105a0a);
    int cmd  = SekReadWord(0x1058fc);

	switch (cmd)
	{
		case 0x100b:
			return 0x0064;

		case 0x6003:
			return data & 0x000f;

		case 0x6004:
			return data & 0x001f;

		case 0x6000:
			return data & 0x0001;

		case 0x0000:
			return data & 0x00ff;

		case 0x6007:
			return data & 0x00ff;

		case 0x8abc:
			/* collision table */
			data = -SekReadWord(0x105818);
			data = ((data / 8 - 4) & 0x1f) * 0x40;
			data += ((SekReadWord(0x105cb0) +
						256*K052109Read(0x1a01) + K052109Read(0x1a00) - 6) / 8 + 12) & 0x3f;
			return data;

		default:
			return 0xffff;
    }
}

static void ssriders_protection_w(int offset)
{
	offset >>= 1;

	if (offset == 1)
	{
		int logical_pri,hardware_pri;

		/* create sprite priority attributes */
		hardware_pri = 1;
		for (logical_pri = 1;logical_pri < 0x100;logical_pri <<= 1)
		{
			int i;

			for (i = 0;i < 128;i++)
			{
				if ((SekReadWord(0x180006 + 128*i) >> 8) == (unsigned short)logical_pri)
				{
					K053245Write(0,((8*i) * 2)+1,hardware_pri);
					hardware_pri++;
				}
			}
		}
	}
}

unsigned char __fastcall Ssriders68KReadByte(unsigned int a)
{
	if (a >= 0x180000 && a <= 0x183fff) {
		int Offset = (a - 0x180000);
		
		if (Offset & (0x31 << 1)) {
			return DrvSpriteRam[Offset ^ 1];
		} else {
			Offset = ((Offset & (0x000e << 1)) >> 1) | ((Offset & (0x1fc0 << 1)) >> 3);
			return K053245ReadWord(0, Offset / 2) >> ((~a & 1) << 3);
		}
	}

	if ((a & 0xffff80) == 0x1c0500) {
		return Drv68KRam[0x4001 ^ (a & 0x7f)];
	}

	if (a >= 0x5a0000 && a <= 0x5a001f) {
		int Offset = (a - 0x5a0000) >> 1;
		Offset &= ~1;
		
		if (a & 1) {
			return K053244Read(0, Offset + 1);
		} else {
			return K053244Read(0, Offset + 0);
		}
	}
	
	if (a >= 0x600000 && a <= 0x603fff) {
		int Offset = (a - 0x600000) >> 1;
		
		if (a & 1) {
			return K052109Read(Offset + 0x2000);
		} else {
			return K052109Read(Offset + 0x0000);
		}
	}
	
	switch (a) {
		case 0x1c0800:
		case 0x1c0801:
			return ssriders_protection_r() >> ((~a & 1) << 3);

		case 0x1c0001: {
			return 0xff - DrvInput[1];
		}
		
		case 0x1c0003: {
			return 0xff - DrvInput[2];
		}
		
		case 0x1c0005: {
			return 0xff - DrvInput[3];
		}
		
		case 0x1c0007: {
			return 0xff - DrvInput[4];
		}
		
		case 0x1c0101: {
			return 0xff - DrvInput[0];
		}
		
		case 0x1c0103: {
			unsigned char Res;
			static int Toggle;

			Res = DrvInput[5];
			if (DrvVBlank) Res |= 0x08;
			Res |= EEPROMRead() & 0x01;
			
			if (InitEEPROMCount) {
				InitEEPROMCount--;
				Res &= 0x7f;
			}
			
			Toggle ^= 0x04;
			
			return Res ^ Toggle;
		}
		
		case 0x1c0401: {
			// watchdog read
			return 0;
		}
		
		case 0x5c0601:
		case 0x5c0603: {
			int Offset = (a - 0x5c0601) >> 1;
			return K053260Read(0, Offset + 2);
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read byte => %06X\n"), a);
		}
	}
	
	return 0;
}

void __fastcall Ssriders68KWriteByte(unsigned int a, unsigned char d)
{
	if (a >= 0x180000 && a <= 0x183fff) {
		int Offset = (a - 0x180000);
		DrvSpriteRam[Offset ^ 1] = d;

		if (!(Offset & (0x31 << 1))) {
			Offset = ((Offset & (0x000e << 1)) >> 1) | ((Offset & (0x1fc0 << 1)) >> 3);

			int data = K053245ReadWord(0, Offset/2);

			if (a & 1) {
				data &= 0xff00;
				data |= d;
			} else {
				data &= 0x00ff;
				data |= d << 8;
			}

			K053245WriteWord(0, Offset/2, data);

		}
		return;
	}

	if ((a & ~3) == 0x1c0800) {
		bprintf (0, _T("protection write byte %x %x\n"), a, d);
		return;
	}

	// extra ram for tmnt2
	if ((a & 0xffff80) == 0x1c0500) {
		Drv68KRam[0x4001 ^ (a & 0x7f)] = d;
		//*((unsigned short*)(Drv68KRam + 0x4000 + (a & 0x7e))) = d;
		return;
	}

	if (a >= 0x5a0000 && a <= 0x5a001f) {
		int Offset = (a - 0x5a0000) >> 1;
		Offset &= ~1;
		
		K053244Write(0, Offset + 1, d & 0xff);
		K053244Write(0, Offset + 0, d >> 8);
		return;
	}
	
	if (a >= 0x5c0700 && a <= 0x5c071f) {
		K053251Write((a - 0x5c0700) >> 1, d);
		return;
	}
	
	if (a >= 0x600000 && a <= 0x603fff) {
		int Offset = (a - 0x600000) >> 1;
		
		if (a & 1) {
			K052109Write(Offset + 0x2000, d);
		} else {
			K052109Write(Offset + 0x0000, d);
		}
		return;
	}
	
	switch (a) {
		case 0x1c0201: {
			EEPROMWrite(d & 0x04, d & 0x02, d & 0x01);
			K053244BankSelect(0, ((d & 0x20) >> 5) << 2);
			dim_c = d & 0x18;
			return;
		}
		
		case 0x1c0301: {
			K052109RMRDLine = d & 0x08;
			dim_v = (d & 0x70) >> 4;
			return;
		}
		
		case 0x5c0601: {
			K053260Write(0, 0, d);
			return;
		}
		
		case 0x5c0605: {
			ZetOpen(0);
			ZetSetVector(0xff);
			ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
			ZetClose();
			return;
		}	
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write byte => %06X, %02X\n"), a, d);
		}
	}
}

unsigned short __fastcall Ssriders68KReadWord(unsigned int a)
{
	if (a >= 0x180000 && a <= 0x183fff) {
		UINT16 *SpriteRam = (UINT16*)DrvSpriteRam;
		int Offset = (a - 0x180000) >> 1;
		
		if (Offset & 0x31) {
			return SpriteRam[Offset];
		} else {
			Offset = ((Offset & 0x000e) >> 1) | ((Offset & 0x1fc0) >> 3);
			return K053245ReadWord(0, Offset);
		}
	}

	if ((a & 0xffff80) == 0x1c0500) {
		return *((unsigned short*)(Drv68KRam + 0x4000 + (a & 0x7e)));
	}

	if (a == 0x1c0800) return ssriders_protection_r();

	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("68K Read word => %06X\n"), a);
		}
	}
		
	return 0;
}

void __fastcall Ssriders68KWriteWord(unsigned int a, unsigned short d)
{
	if (a >= 0x180000 && a <= 0x183fff) {
		UINT16 *SpriteRam = (UINT16*)DrvSpriteRam;
		int Offset = (a - 0x180000) >> 1;
		SpriteRam[Offset] = d;
		
		if (!(Offset & 0x0031)) {
			Offset = ((Offset & 0x000e) >> 1) | ((Offset & 0x1fc0) >> 3);
			K053245WriteWord(0, Offset, d);
		}
		return;
	}
	
	if (a >= 0x5a0000 && a <= 0x5a001f) {
		int Offset = (a - 0x5a0000) >> 1;
		Offset &= ~1;
		
		K053244Write(0, Offset + 1, d & 0xff);
		K053244Write(0, Offset + 0, d >> 8);
		return;
	}

	if ((a & 0xffff80) == 0x1c0500) {
		*((unsigned short*)(Drv68KRam + 0x4000 + (a & 0x7e))) = d;
		return;
	}

	if (a == 0x1c0802) {
		ssriders_protection_w(2);
		return;
	}

	switch (a) {
		case 0x1c0800:
		case 0x1c0802: {
			ssriders_protection_w(a - 0x1c0800);
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("68K Write word => %06X, %04X\n"), a, d);
		}
	}
}

unsigned char __fastcall Thndrx268KReadByte(unsigned int a)
{
	K052109WordNoA12Read(0x600000)
	K051937ByteRead(0x700000)

	if ((a & 0xffffc00) == 0x700400) {
		return K051960Read(a & 0x3ff);
	}

	if (a >= 0x500000 && a <= 0x50003f) {
		int Offset = (a - 0x500000) >> 1;
		if (a & 1) {
			return K054000Read(Offset);
		} else {
			return 0;
		}
	}

	switch (a)
	{
		case 0x400001:
		case 0x400003:
			return K053260Read(0, ((a & 2) >> 1) + 2);

		case 0x500200:
		{
			int ret = DrvInput[1]^0xff;
			if (InitEEPROMCount) {
				ret &= 0xf7;
				InitEEPROMCount--;
			}
			return ret;
		}

		case 0x500201:
			return DrvInput[0]^0xff;

		case 0x500202:
		{
			static int toggle;
			toggle ^= 0x08;
			int ret = 0xfe;
			ret |= (EEPROMRead() & 1);
			ret ^= toggle;

			return ret;
		}

		case 0x500203:
			return DrvInput[2]^0xff;
	}
		

	return 0;
}

void __fastcall Thndrx268KWriteWord(unsigned int a, unsigned short d)
{
	if ((a & 0xffffc00) == 0x700400) {
		K051960Write((a & 0x3fe) | 0, d >> 8);
		K051960Write((a & 0x3fe) | 1, d & 0xff);
		return;
	}
}

void __fastcall Thndrx268KWriteByte(unsigned int a, unsigned char d)
{
	K052109WordNoA12Write(0x600000)
	K015937ByteWrite(0x700000)

	if ((a & 0xffffc00) == 0x700400) {
		K051960Write(a & 0x3ff, d);
		return;
	}
	
	if (a >= 0x300000 && a <= 0x30001f) {
		if (a & 1) K053251Write((a - 0x300000) >> 1, d);
		return;
	}

	if (a >= 0x500000 && a <= 0x50003f) {
		int Offset = (a - 0x500000) >> 1;
		if (a & 1) K054000Write(Offset, d);
		return;
	}

	switch (a)
	{
		case 0x400001:
			K053260Write(0, 0, d);
		return;

		case 0x500101:
		{
			EEPROMWriteBit(d & 0x01);
			EEPROMSetCSLine((d & 0x02) ? EEPROM_CLEAR_LINE : EEPROM_ASSERT_LINE);
			EEPROMSetClockLine((d & 0x04) ? EEPROM_ASSERT_LINE : EEPROM_CLEAR_LINE);

		//	EEPROMWrite(d & 0x04, d & 0x02, d & 0x01);

			static int last;
			if (last == 0 && (d & 0x20) != 0) {
				ZetSetVector(0xff);
				ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
			}
			last = d & 0x20;

			K052109RMRDLine = d & 0x40;
		}
		return;
	}
}

unsigned short __fastcall Lgtnfght68KReadWord(unsigned int a)
{
	if (a >= 0xb0000 && a <= 0xb3fff) {
		UINT16 *SpriteRam = (UINT16*)DrvSpriteRam;
		int Offset = (a - 0xb0000) >> 1;
		
		if (Offset & 0x31) {
			return SpriteRam[Offset];
		} else {
			Offset = ((Offset & 0x000e) >> 1) | ((Offset & 0x1fc0) >> 3);
			return K053245ReadWord(0, Offset);
		}
	}

	return 0;
}

unsigned char __fastcall Lgtnfght68KReadByte(unsigned int a)
{
	K052109WordNoA12Read(0x100000)

	if (a >= 0xb0000 && a <= 0xb3fff) {
		int Offset = (a - 0xb0000);
		
		if (Offset & (0x31 << 1)) {
			return DrvSpriteRam[Offset ^ 1];
		} else {
			Offset = ((Offset & (0x000e << 1)) >> 1) | ((Offset & (0x1fc0 << 1)) >> 3);
			return K053245ReadWord(0, Offset / 2) >> ((~a & 1) << 3);
		}
	}
		
	if (a >= 0xc0000 && a <= 0xc001f) {
		int Offset = (a - 0xc0000) >> 1;
		Offset &= ~1;
		
		if (a & 1) {
			return K053244Read(0, Offset + 1);
		} else {
			return K053244Read(0, Offset + 0);
		}
	}

	switch (a)
	{
		case 0x0a0001:
			return ~DrvInput[0] & 0xfb;

		case 0x0a0003:
			return ~DrvInput[1];

		case 0x0a0005:
			return ~DrvInput[2];

		case 0x0a0007:
			return DrvDip[0];

		case 0x0a0009:
			return DrvDip[1];

		case 0x0a0011:
			return DrvDip[2];

		case 0x0a0021:
		case 0x0a0023:
			return K053260Read(0, ((a & 2) >> 1) + 2); // k053260
	}

	return 0;
}

void __fastcall Lgtnfght68KWriteWord(unsigned int a, unsigned short d)
{
	if (a >= 0xb0000 && a <= 0xb3fff) {
		UINT16 *SpriteRam = (UINT16*)DrvSpriteRam;
		int Offset = (a - 0xb0000) >> 1;
		SpriteRam[Offset] = d;
		
		if (!(Offset & 0x0031)) {
			Offset = ((Offset & 0x000e) >> 1) | ((Offset & 0x1fc0) >> 3);
			K053245WriteWord(0, Offset, d);
		}
		return;
	}
}

void __fastcall Lgtnfght68KWriteByte(unsigned int a, unsigned char d)
{
	K052109WordNoA12Write(0x100000)

	if (a >= 0x0e0000 && a <= 0x0e001f) {
		K053251Write((a - 0x0e0000) >> 1, d);
		return;
	}

	if (a >= 0xb0000 && a <= 0xb3fff) {
		int Offset = (a - 0xb0000);
		DrvSpriteRam[Offset ^ 1] = d;

		if (!(Offset & (0x31 << 1))) {
			Offset = ((Offset & (0x000e << 1)) >> 1) | ((Offset & (0x1fc0 << 1)) >> 3);

			int data = K053245ReadWord(0, Offset/2);

			if (a & 1) {
				data &= 0xff00;
				data |= d;
			} else {
				data &= 0x00ff;
				data |= d << 8;
			}

			K053245WriteWord(0, Offset/2, data);

		}
		return;
	}

	if (a >= 0xc0000 && a <= 0xc001f) {
		int Offset = (a - 0xc0000) >> 1;
		Offset &= ~1;
		
		K053244Write(0, Offset + 1, d & 0xff);
		K053244Write(0, Offset + 0, d >> 8);
		return;
	}

	switch (a)
	{
		case 0xa0019:
		{
			static int last;
			if (last == 0 && (d & 0x4) != 0) {
				ZetSetVector(0xff);
				ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
			}
			last = d & 0x4;

			K052109RMRDLine = d & 0x8;
		}
		return;

		case 0xa0021:
			K053260Write(0, 0, d);
		return;

		case 0xa0029: // watchdog
		return;
	}
}

unsigned short __fastcall Punkshot68KReadWord(unsigned int a)
{
	if ((a & 0xfffc00) == 0x110400) {
		return K051960Read((a & 0x3fe) | 1) | (K051960Read((a & 0x3fe) | 0) << 8);
	}

	return 0;
}

unsigned char __fastcall Punkshot68KReadByte(unsigned int a)
{
	K052109WordNoA12Read(0x100000)
	K051937ByteRead(0x110000)
	K051960ByteRead(0x110400)

	switch (a)
	{
		case 0x0a0000:
			return DrvDip[1];

		case 0x0a0001:
			return DrvDip[0];

		case 0x0a0003:
			return ~DrvInput[0];

		case 0x0a0002:
			return DrvDip[2] | 0x0f;

		case 0x0a0005:
			return ~DrvInput[3];

		case 0x0a0004:
			return ~DrvInput[4];

		case 0x0a0007:
			return ~DrvInput[1];

		case 0x0a0006:
			return ~DrvInput[2];

		case 0x0a0041:
		case 0x0a0043:
			return K053260Read(0, ((a & 2) >> 1) | 2);

		case 0xfffffc:
		case 0xfffffd:
		case 0xfffffe:
		case 0xffffff:
			return rand();
	}

	return 0;
}

void __fastcall Punkshot68KWriteWord(unsigned int a, unsigned short d)
{
	if ((a & 0xfffc00) == 0x110400) {
		K051960Write((a & 0x3fe) | 0, d >> 8);
		K051960Write((a & 0x3fe) | 1, d & 0xff);

		return;
	}

	if ((a & 0xff8000) == 0x100000) {
		SekWriteByte(a+1, d & 0xff);
		return;
	}
}

void __fastcall Punkshot68KWriteByte(unsigned int a, unsigned char d)
{
	K052109WordNoA12Write(0x100000)
	K015937ByteWrite(0x110000)
	K051960ByteWrite(0x110400)

	if ((a & 0xffffe1) == 0xa0061) {
		K053251Write((a & 0x1f) >> 1, d);
		return;
	}

	switch (a)
	{
		case 0x0a0020:
		case 0x0a0021:
		{
			static int last;
			K052109RMRDLine = d & 0x08;

			if (last == 0x04 && (d & 0x04) == 0x00) {
				ZetSetVector(0xff);
				ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
			}

			last = d & 0x04; 
		}
		return;

		case 0x0a0041:
			K053260Write(0, 0, d);
		return;

		case 0x0a0081:
			// watchdog
		return;
	}
}

unsigned char __fastcall TmntZ80Read(unsigned short a)
{
	if (a >= 0xb000 && a <= 0xb00d) {
		return K007232ReadReg(0, a - 0xb000);
	}
	
	switch (a) {
		case 0x9000: {
			return TitleSoundLatch;
		}
		
		case 0xa000: {
			ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
			return DrvSoundLatch;
		}
		
		case 0xc001: {
			return BurnYM2151ReadStatus();
		}
		
		case 0xf000: {
			return UPD7759BusyRead(0);
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Read => %04X\n"), a);
		}
	}

	return 0;
}

void __fastcall TmntZ80Write(unsigned short a, unsigned char d)
{
	if (a >= 0xb000 && a <= 0xb00d) {
		K007232WriteReg(0, (a - 0xb000), d);
		return;
	}
	
	switch (a) {
		case 0x9000: {
			TitleSoundLatch = d;
			if (d & 0x04) {
				PlayTitleSample = 1;
			} else {
				PlayTitleSample = 0;
				TitleSamplePos = 0;
			}
			
			UPD7759ResetWrite(0, d & 2);
			return;
		}
		
		case 0xc000: {
			BurnYM2151SelectRegister(d);
			return;
		}
		
		case 0xc001: {
			BurnYM2151WriteRegister(d);
			return;
		}
		
		case 0xd000: {
			UPD7759PortWrite(0, d);
			return;
		}
		
		case 0xe000: {
			UPD7759StartWrite(0, d);
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Write => %04X, %02X\n"), a, d);
		}
	}
}

unsigned char __fastcall MiaZ80Read(unsigned short a)
{
	if (a >= 0xb000 && a <= 0xb00d) {
		return K007232ReadReg(0, a - 0xb000);
	}
	
	switch (a) {
		case 0xa000: {
			ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
			return DrvSoundLatch;
		}
		
		case 0xc001: {
			return BurnYM2151ReadStatus();
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Read => %04X\n"), a);
		}
	}

	return 0;
}

void __fastcall MiaZ80Write(unsigned short a, unsigned char d)
{
	if (a >= 0xb000 && a <= 0xb00d) {
		K007232WriteReg(0, (a - 0xb000), d);
		return;
	}
	
	switch (a) {
		case 0xc000: {
			BurnYM2151SelectRegister(d);
			return;
		}
		
		case 0xc001: {
			BurnYM2151WriteRegister(d);
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Write => %04X, %02X\n"), a, d);
		}
	}
}

unsigned char __fastcall BlswhstlZ80Read(unsigned short a)
{
	if (a >= 0xfa00 && a <= 0xfa2f) {
		if (a == 0xfa00) ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
		return K053260Read(0, a - 0xfa00);
	}
	
	switch (a) {
		case 0xf801: {
			return BurnYM2151ReadStatus();
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Read => %04X\n"), a);
		}
	}

	return 0;
}

void __fastcall BlswhstlZ80Write(unsigned short a, unsigned char d)
{
	if (a >= 0xfa00 && a <= 0xfa2f) {
		K053260Write(0, (a - 0xfa00), d);
		return;
	}
	
	switch (a) {
		case 0xf800: {
			BurnYM2151SelectRegister(d);
			return;
		}
		
		case 0xf801: {
			BurnYM2151WriteRegister(d);
			return;
		}
		
		case 0xfc00: { // tmnt2 needs this
			ZetRun(100);
			ZetNmi();
			return;
		}

		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Write => %04X, %02X\n"), a, d);
		}
	}
}

void __fastcall Thndrx2Z80Write(unsigned short a, unsigned char d)
{
	if (a >= 0xfc00 && a <= 0xfc2f) {
		K053260Write(0, (a - 0xfc00), d);
		return;
	}

	switch (a)
	{
		case 0xf800:
		case 0xf810:
			BurnYM2151SelectRegister(d);
		return;

		case 0xf801:
		case 0xf811:
			BurnYM2151WriteRegister(d);
		return;

		case 0xfa00:
			ZetRun(100);
			ZetNmi();
		return;
	}
}

unsigned char __fastcall Thndrx2Z80Read(unsigned short a)
{
	if (a >= 0xfc00 && a <= 0xfc2f) {
		if (a == 0xfc00) ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
		return K053260Read(0, a - 0xfc00);
	}

	switch (a)
	{
		case 0xf801:
		case 0xf811:
			return BurnYM2151ReadStatus();
	}

	return 0;
}

void __fastcall LgtnfghtZ80Write(unsigned short a, unsigned char d)
{
	if (a >= 0xc000 && a <= 0xc02f) {
		K053260Write(0, (a - 0xc000), d);
		return;
	}

	switch (a)
	{
		case 0xa000:
			BurnYM2151SelectRegister(d);
		return;

		case 0xa001:
			BurnYM2151WriteRegister(d);
		return;
	}
}

unsigned char __fastcall LgtnfghtZ80Read(unsigned short a)
{
	if (a >= 0xc000 && a <= 0xc02f) {
		if (a == 0xc000) ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
		return K053260Read(0, a - 0xc000);
	}

	switch (a)
	{
		case 0xa001:
			return BurnYM2151ReadStatus();
	}

	return 0;
}

static void shuffle(UINT16 *buf, int len)
{
	int i;
	UINT16 t;

	if (len == 2) return;

	if (len % 4) return;

	len /= 2;

	for (i = 0;i < len/2;i++) {
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}

	shuffle(buf,len);
	shuffle(buf + len,len);
}

static void byte_shuffle(UINT8 *buf, int len)
{
	int i;
	UINT8 t;

	if (len == 2) return;

	if (len % 4) return;

	len /= 2;

	for (i = 0;i < len/2;i++) {
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}

	byte_shuffle(buf,len);
	byte_shuffle(buf + len,len);
}

static void TmntUnscrambleGfx(unsigned char *pSrc, int nLength)
{
	int bits[32];
	for (int i = 0; i < nLength; i += 4) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 8; k++) {
				bits[8*j + k] = (pSrc[i + j] >> k) & 1;
			}
		}

		for (int j = 0; j < 4; j++) {
			pSrc[i + j] = 0;
			for (int k = 0; k < 8; k++) {
				pSrc[i + j] |= bits[j + 4*k] << k;
			}
		}
	}
}

static void TmntUnscrambleSprites()
{
	BurnLoadRom(DrvTempRom + 0x200000, 11, 1);
	unsigned char *SpriteConvTable = DrvTempRom + 0x200000;
	
	memcpy(DrvTempRom, DrvSpriteRom, 0x200000);
	
	for (int A = 0; A < 0x80000; A++) {
		int B, i, entry;
		int bits[10];

		static const UINT8 bit_pick_table[10][8] = {
			{ 3,   3,   3,   3,   3,   3,   3,   3 },
			{ 0,   0,   5,   5,   5,   5,   5,   5 },
			{ 1,   1,   0,   0,   0,   7,   7,   7 },
			{ 2,   2,   1,   1,   1,   0,   0,   9 },
			{ 4,   4,   2,   2,   2,   1,   1,   0 },
			{ 5,   6,   4,   4,   4,   2,   2,   1 },
			{ 6,   5,   6,   6,   6,   4,   4,   2 },
			{ 7,   7,   7,   7,   8,   6,   6,   4 },
			{ 8,   8,   8,   8,   7,   8,   8,   6 },
			{ 9,   9,   9,   9,   9,   9,   9,   8 }
		};

		entry = SpriteConvTable[(A & 0x7f800) >> 11] & 7;

		for (i = 0;i < 10;i++)
			bits[i] = (A >> i) & 0x01;

		B = A & 0x7fc00;

		for (i = 0; i < 10;i++) B |= bits[bit_pick_table[i][entry]] << i;

		DrvSpriteRom[4*A+0] = DrvTempRom[4*B+0];
		DrvSpriteRom[4*A+1] = DrvTempRom[4*B+1];
		DrvSpriteRom[4*A+2] = DrvTempRom[4*B+2];
		DrvSpriteRom[4*A+3] = DrvTempRom[4*B+3];
	}
}

static void MiaUnscrambleSprites()
{
	memcpy(DrvTempRom, DrvSpriteRom, 0x100000);
	
	for (int A = 0; A < 0x40000; A++) {
		int B, i;
		int bits[8];
		for (i = 0; i < 8; i++) bits[i] = (A >> i) & 0x01;

		B = A & 0x3ff00;

		if ((A & 0x3c000) == 0x3c000) {
			B |= bits[3] << 0;
			B |= bits[5] << 1;
			B |= bits[0] << 2;
			B |= bits[1] << 3;
			B |= bits[2] << 4;
			B |= bits[4] << 5;
			B |= bits[6] << 6;
			B |= bits[7] << 7;
		} else {
			B |= bits[3] << 0;
			B |= bits[5] << 1;
			B |= bits[7] << 2;
			B |= bits[0] << 3;
			B |= bits[1] << 4;
			B |= bits[2] << 5;
			B |= bits[4] << 6;
			B |= bits[6] << 7;
		}

		DrvSpriteRom[4*A+0] = DrvTempRom[4*B+0];
		DrvSpriteRom[4*A+1] = DrvTempRom[4*B+1];
		DrvSpriteRom[4*A+2] = DrvTempRom[4*B+2];
		DrvSpriteRom[4*A+3] = DrvTempRom[4*B+3];
	}
}

static void TmntDecodeTitleSample()
{
	for (int i = 0; i < 0x40000; i++) {
		int val = DrvTempRom[2 * i + 0] + (DrvTempRom[(2 * i) + 1] << 8);
		int expo = val >> 13;

	  	val = (val >> 3) & (0x3ff);
		val -= 0x200;

		val <<= (expo-3);

		DrvTitleSample[i] = val;
	}
}

static int TilePlaneOffsets[4]     = { 24, 16, 8, 0 };
static int TileXOffsets[8]         = { 0, 1, 2, 3, 4, 5, 6, 7 };
static int TileYOffsets[8]         = { 0, 32, 64, 96, 128, 160, 192, 224 };
static int SpritePlaneOffsets[4]   = { 24, 16, 8, 0 };
static int SpritePlaneOffsets2[4]  = { 0, 8, 16, 24 };
static int SpriteXOffsets[16]      = { 0, 1, 2, 3, 4, 5, 6, 7, 256, 257, 258, 259, 260, 261, 262, 263 };
static int SpriteYOffsets[16]      = { 0, 32, 64, 96, 128, 160, 192, 224, 512, 544, 576, 608, 640, 672, 704, 736 };

static void K052109TmntCallback(int Layer, int Bank, int *Code, int *Colour, int* /*xFlip*/, int *)
{
	*Code |= ((*Colour & 0x03) << 8) | ((*Colour & 0x10) << 6) | ((*Colour & 0x0c) << 9) | (Bank << 13);
	*Colour = LayerColourBase[Layer] + ((*Colour & 0xe0) >> 5);
}

static void K052109MiaCallback(int Layer, int Bank, int *Code, int *Colour, int *xFlip, int *)
{
	*xFlip = *Colour & 0x04;
	
	if (Layer == 0) {
		*Code |= ((*Colour & 0x01) << 8);
		*Colour = LayerColourBase[Layer] + ((*Colour & 0x80) >> 5) + ((*Colour & 0x10) >> 1);
	} else {
		*Code |= ((*Colour & 0x01) << 8) | ((*Colour & 0x18) << 6) | (Bank << 11);
		*Colour = LayerColourBase[Layer] + ((*Colour & 0xe0) >> 5);
	}
}

static void K052109CuebrickCallback(int Layer, int /*Bank*/, int *Code, int *Colour, int */*xFlip*/, int *)
{
	if (K052109RMRDLine == 0 && Layer == 0) {
		*Code |= ((*Colour & 0x01) << 8);
		*Colour = LayerColourBase[Layer] + ((*Colour & 0x80) >> 5) + ((*Colour & 0x10) >> 1);
	} else {
		*Code |= ((*Colour & 0x0f) << 8);
		*Colour = LayerColourBase[Layer] + ((*Colour & 0xe0) >> 5);
	}
}

static void K052109BlswhstlCallback(int Layer, int Bank, int *Code, int *Colour, int */*Flags*/, int */*Priority*/)
{
	*Code |= ((*Colour & 0x01) << 8) | ((*Colour & 0x10) << 5) | ((*Colour & 0x0c) << 8) | (Bank << 12) | (BlswhstlTileRomBank << 14);
	*Colour = LayerColourBase[Layer] + ((*Colour & 0xe0) >> 5);
}

static void K051960TmntCallback(int *Code, int *Colour, int* /*Priority*/, int* /*Shadow*/)
{
	*Code |= (*Colour & 0x10) << 9;
	*Colour = SpriteColourBase + (*Colour & 0x0f);
}

static void K051960MiaCallback(int* /*Code*/, int *Colour, int* /*Priority*/, int* /*Shadow*/)
{
	*Colour = SpriteColourBase + (*Colour & 0x0f);
}

static void K051960CuebrickCallback(int* Code, int *Colour, int* /*Priority*/, int* /*Shadow*/)
{
	*Colour = SpriteColourBase + (*Colour & 0x0f);
	*Code &= 0xfff;
}

static void K051960Thndrx2Callback(int* code, int *color, int* priority, int* /*shadow*/)
{
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= LayerPri[2])					*priority = 0;
	else if (pri > LayerPri[2] && pri <= LayerPri[1])	*priority = 1;
	else if (pri > LayerPri[1] && pri <= LayerPri[0])	*priority = 2;
	else 							*priority = 3;

	*code &= 0x1fff;
	*color = SpriteColourBase + (*color & 0x0f);
}

static void K051960PunkshotCallback(int *code, int *color, int *priority, int *)
{
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= LayerPri[2])					*priority = 0;
	else if (pri > LayerPri[2] && pri <= LayerPri[1])	*priority = 1;
	else if (pri > LayerPri[1] && pri <= LayerPri[0])	*priority = 2;
	else 							*priority = 3;

	*code |= (*color & 0x10) << 9;
	*code &= 0x3fff;
	*color = SpriteColourBase + (*color & 0x0f);
}

static void K053245BlswhstlCallback(int *Code, int *Colour, int *Priority)
{
	int Pri = 0x20 | ((*Colour & 0x60) >> 2);
	if (Pri <= LayerPri[2])                           	*Priority = 0;
	else if (Pri > LayerPri[2] && Pri <= LayerPri[1]) 	*Priority = 1;
	else if (Pri > LayerPri[1] && Pri <= LayerPri[0]) 	*Priority = 2;
	else 	                                          	*Priority = 3;

	*Code &= 0x1fff;
	*Colour = SpriteColourBase + (*Colour & 0x1f);
}

static void K053245SsridersCallback(int */*Code*/, int *Colour, int *Priority)
{
	int Pri = 0x20 | ((*Colour & 0x60) >> 2);
	if (Pri <= LayerPri[2])                           	*Priority = 0;
	else if (Pri > LayerPri[2] && Pri <= LayerPri[1]) 	*Priority = 1;
	else if (Pri > LayerPri[1] && Pri <= LayerPri[0])	*Priority = 2;
	else 	                                          	*Priority = 3;

	*Colour = SpriteColourBase + (*Colour & 0x1f);
}

static void K053245LgtnfghtCallback(int *code, int *color, int *priority)
{
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= LayerPri[2])					*priority = 0;
	else if (pri > LayerPri[2] && pri <= LayerPri[1])	*priority = 1;
	else if (pri > LayerPri[1] && pri <= LayerPri[0])	*priority = 2;
	else 							*priority = 3;

	*color = SpriteColourBase + (*color & 0x1f);
	*code &= 0x3fff;
}

static void DrvK007232VolCallback(int v)
{
	K007232SetVolume(0, 0, (v >> 4) * 0x11, 0);
	K007232SetVolume(0, 1, 0, (v & 0x0f) * 0x11);
}

static void CuebrickYM2151IrqHandler(int Irq)
{
	CuebrickSndIrqFire = Irq;
}

static int TmntInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	TmntMemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	TmntMemIndex();
	
	K052109Init(DrvTileRom, 0x0fffff);
	K052109SetCallback(K052109TmntCallback);
	K051960Init(DrvSpriteRom, 0x1fffff);
	K051960SetCallback(K051960TmntCallback);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x040001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x040000, 3, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x080000, 6, 1); if (nRet != 0) return 1;
	shuffle((UINT16*)DrvTileRom, 0x080000);
	TmntUnscrambleGfx(DrvTileRom, 0x100000);
	GfxDecode(0x100000 / 32, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	DrvTempRom = (unsigned char *)malloc(0x200100);
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x080000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x100000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x180000, 10, 1); if (nRet != 0) return 1;
	shuffle((UINT16*)DrvSpriteRom, 0x100000);
	TmntUnscrambleGfx(DrvSpriteRom, 0x200000);
	TmntUnscrambleSprites();
	
	// Decode the sprites
	GfxDecode(0x200000 / 128, 4, 16, 16, SpritePlaneOffsets, SpriteXOffsets, SpriteYOffsets, 0x400, DrvSpriteRom, DrvSprites);	
	
	// Load K007232 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 13, 1); if (nRet != 0) return 1;	
	
	// Load UPD7759C Sample Roms
	nRet = BurnLoadRom(DrvUPD7759CRom + 0x00000, 14, 1); if (nRet != 0) return 1;
	
	// Load title sample
	memset(DrvTempRom, 0, 0x080000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 15, 1); if (nRet != 0) return 1;
	TmntDecodeTitleSample();
	
	free(DrvTempRom);
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x05ffff, SM_ROM);
	SekMapMemory(Drv68KRam           , 0x060000, 0x063fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x080000, 0x080fff, SM_RAM);
	SekSetReadWordHandler(0, Tmnt68KReadWord);
	SekSetWriteWordHandler(0, Tmnt68KWriteWord);
	SekSetReadByteHandler(0, Tmnt68KReadByte);
	SekSetWriteByteHandler(0, Tmnt68KWriteByte);
	SekClose();
	
	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(TmntZ80Read);
	ZetSetWriteHandler(TmntZ80Write);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80Rom                );
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80Ram                );
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80Ram                );
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();
	
	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K007232Init(0, 3579545, DrvSoundRom, 0x20000);
	K007232SetPortWriteHandler(0, DrvK007232VolCallback);
	
	UPD7759Init(0, UPD7759_STANDARD_CLOCK, DrvUPD7759CRom);
	
	GenericTilesInit();
	
	LayerColourBase[0] = 0;
	LayerColourBase[1] = 32;
	LayerColourBase[2] = 40;
	SpriteColourBase = 16;

	// Reset the driver
	TmntDoReset();

	return 0;
}

static int MiaInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	MiaMemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MiaMemIndex();
	
	K052109Init(DrvTileRom, 0x03ffff);
	K052109SetCallback(K052109MiaCallback);
	K051960Init(DrvSpriteRom, 0x0fffff);
	K051960SetCallback(K051960MiaCallback);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 2, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 3, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x000001, 4, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x020000, 5, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x020001, 6, 2); if (nRet != 0) return 1;
	shuffle((UINT16*)DrvTileRom, 0x020000);
	TmntUnscrambleGfx(DrvTileRom, 0x040000);
	GfxDecode(0x2000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	DrvTempRom = (unsigned char *)malloc(0x100000);
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x080000,  8, 1); if (nRet != 0) return 1;
	shuffle((UINT16*)DrvSpriteRom, 0x080000);
	TmntUnscrambleGfx(DrvSpriteRom, 0x100000);
	MiaUnscrambleSprites();
	
	// Decode the sprites
	GfxDecode(0x2000, 4, 16, 16, SpritePlaneOffsets, SpriteXOffsets, SpriteYOffsets, 0x400, DrvSpriteRom, DrvSprites);	
	
	// Load K007232 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 10, 1); if (nRet != 0) return 1;
	
	free(DrvTempRom);
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x03ffff, SM_ROM);
	SekMapMemory(Drv68KRam + 0x0000  , 0x040000, 0x043fff, SM_RAM);
	SekMapMemory(Drv68KRam + 0x4000  , 0x060000, 0x063fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x080000, 0x080fff, SM_RAM);
	SekSetReadWordHandler(0, Mia68KReadWord);
	SekSetWriteWordHandler(0, Mia68KWriteWord);
	SekSetReadByteHandler(0, Mia68KReadByte);
	SekSetWriteByteHandler(0, Mia68KWriteByte);
	SekClose();
	
	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(MiaZ80Read);
	ZetSetWriteHandler(MiaZ80Write);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80Rom                );
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80Ram                );
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80Ram                );
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();
	
	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K007232Init(0, 3579545, DrvSoundRom, 0x20000);
	K007232SetPortWriteHandler(0, DrvK007232VolCallback);
	
	GenericTilesInit();
	
	LayerColourBase[0] = 0;
	LayerColourBase[1] = 32;
	LayerColourBase[2] = 40;
	SpriteColourBase = 16;

	// Reset the driver
	DrvDoReset();

	return 0;
}

static int CuebrickInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	CuebrickMemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	CuebrickMemIndex();
	
	K052109Init(DrvTileRom, 0x03ffff);
	K052109SetCallback(K052109CuebrickCallback);

	K051960Init(DrvSpriteRom, 0x03ffff);
	K051960SetCallback(K051960CuebrickCallback);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x000001, 3, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x020000, 4, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x020001, 5, 2); if (nRet != 0) return 1;
	byte_shuffle(DrvTileRom, 0x040000);
	GfxDecode(0x2000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000, 6, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x000001, 7, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x020000, 8, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x020001, 9, 2); if (nRet != 0) return 1;
	byte_shuffle(DrvSpriteRom, 0x040000);
	GfxDecode(0x0800, 4, 16, 16, SpritePlaneOffsets, SpriteXOffsets, SpriteYOffsets, 0x400, DrvSpriteRom, DrvSprites);	
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x01ffff, SM_ROM);
	SekMapMemory(Drv68KRam + 0x0000  , 0x040000, 0x043fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x080000, 0x080fff, SM_RAM);
	SekSetReadWordHandler(0, Cuebrick68KReadWord);
	SekSetWriteWordHandler(0, Cuebrick68KWriteWord);
	SekSetReadByteHandler(0, Cuebrick68KReadByte);
	SekSetWriteByteHandler(0, Cuebrick68KWriteByte);
	SekClose();
	
	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	BurnYM2151SetIrqHandler(&CuebrickYM2151IrqHandler);
	
	GenericTilesInit();
	
	LayerColourBase[0] = 0;
	LayerColourBase[1] = 32;
	LayerColourBase[2] = 40;
	SpriteColourBase = 16;

	// Reset the driver
	CuebrickDoReset();

	return 0;
}

static int BlswhstlInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	BlswhstlMemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	BlswhstlMemIndex();
	
	K052109Init(DrvTileRom, 0x0fffff);
	K052109SetCallback(K052109BlswhstlCallback);
	K052109AdjustScroll(8, 0);

	K053245Init(0, DrvSpriteRom, 0xfffff, K053245BlswhstlCallback);
	K053245SetSpriteOffset(0, -112, 16);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x040001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x040000, 3, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x080000, 6, 1); if (nRet != 0) return 1;
	BurnByteswap(DrvTileRom, 0x100000);
	konami_rom_deinterleave_2(DrvTileRom, 0x100000);
	GfxDecode(0x8000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x080000,  8, 1); if (nRet != 0) return 1;
	BurnByteswap(DrvSpriteRom, 0x100000);
	konami_rom_deinterleave_2(DrvSpriteRom, 0x100000);
	K053245GfxDecode(DrvSpriteRom, DrvSprites, 0x100000);
	
	// Load K053260 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 9, 1); if (nRet != 0) return 1;
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x07ffff, SM_ROM);
	SekMapMemory(Drv68KRam           , 0x204000, 0x207fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x400000, 0x400fff, SM_RAM);
	SekSetReadWordHandler(0, Blswhstl68KReadWord);
	SekSetWriteWordHandler(0, Blswhstl68KWriteWord);
	SekSetReadByteHandler(0, Blswhstl68KReadByte);
	SekSetWriteByteHandler(0, Blswhstl68KWriteByte);
	SekClose();
	
	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(BlswhstlZ80Read);
	ZetSetWriteHandler(BlswhstlZ80Write);
	ZetMapArea(0x0000, 0xefff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0xefff, 2, DrvZ80Rom                );
	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 1, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();
	
	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K053260Init(0, 3579545, DrvSoundRom, 0x100000);

	EEPROMInit(&BlswhstlEEPROMInterface);
	
	GenericTilesInit();
	
	// Reset the driver
	BlswhstlDoReset();

	return 0;
}

static int SsridersInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	SsridersMemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	SsridersMemIndex();
	
	K052109Init(DrvTileRom, 0x0fffff);
	K052109SetCallback(K052109TmntCallback);
	K052109AdjustScroll(8, 0);

	K053245Init(0, DrvSpriteRom, 0x1fffff, K053245LgtnfghtCallback);
	K053245SetSpriteOffset(0, -368, 528);
	K05324xSetZRejection(0);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x080001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x080000, 3, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x080000, 6, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvTileRom, 0x100000);
	GfxDecode(0x8000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x100000,  8, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvSpriteRom, 0x200000);
	K053245GfxDecode(DrvSpriteRom, DrvSprites, 0x200000);
	
	// Load K053260 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 9, 1); if (nRet != 0) return 1;
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x0bffff, SM_ROM);
	SekMapMemory(Drv68KRam           , 0x104000, 0x107fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x140000, 0x140fff, SM_RAM);
	SekSetReadWordHandler(0, Ssriders68KReadWord);
	SekSetWriteWordHandler(0, Ssriders68KWriteWord);
	SekSetReadByteHandler(0, Ssriders68KReadByte);
	SekSetWriteByteHandler(0, Ssriders68KWriteByte);
	SekClose();
	
	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(BlswhstlZ80Read);
	ZetSetWriteHandler(BlswhstlZ80Write);
	ZetMapArea(0x0000, 0xefff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0xefff, 2, DrvZ80Rom                );
	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 1, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();
	
	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K053260Init(0, 3579545, DrvSoundRom, 0x100000);

	EEPROMInit(&BlswhstlEEPROMInterface);
	
	GenericTilesInit();
	
	// Reset the driver
	SsridersDoReset();

	return 0;
}

static int Thndrx2Init()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	Thndrx2MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	Thndrx2MemIndex();
	
	K052109Init(DrvTileRom, 0x0fffff);
	K052109SetCallback(K052109TmntCallback);

	K051960Init(DrvSpriteRom, 0x0fffff);
	K051960SetCallback(K051960Thndrx2Callback);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 2, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 3, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x080000, 4, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvTileRom, 0x100000);
	GfxDecode(0x8000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,  5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x080000,  6, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvSpriteRom, 0x100000);
	GfxDecode(0x2000, 4, 16, 16, SpritePlaneOffsets2, SpriteXOffsets, SpriteYOffsets, 0x400, DrvSpriteRom, DrvSprites);
	
	// Load K053260 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 7, 1); if (nRet != 0) return 1;

	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x03ffff, SM_ROM);
	SekMapMemory(Drv68KRam           , 0x100000, 0x103fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x200000, 0x200fff, SM_RAM);
	SekSetWriteWordHandler(0, Thndrx268KWriteWord);
	SekSetReadByteHandler(0, Thndrx268KReadByte);
	SekSetWriteByteHandler(0, Thndrx268KWriteByte);
	SekClose();

	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(Thndrx2Z80Read);
	ZetSetWriteHandler(Thndrx2Z80Write);
	ZetMapArea(0x0000, 0xefff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0xefff, 2, DrvZ80Rom                );
	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 1, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();

	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K053260Init(0, 3579545, DrvSoundRom, 0x80000);

	EEPROMInit(&thndrx2_eeprom_interface);
	
	GenericTilesInit();
	
	// Reset the driver
	SsridersDoReset();

	return 0;
}

static int LgtnfghtInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	LgtnfghtMemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	LgtnfghtMemIndex();
	
	K052109Init(DrvTileRom, 0x0fffff);
	K052109SetCallback(K052109TmntCallback);
	K052109AdjustScroll(8, 0);

	K053245Init(0, DrvSpriteRom, 0x0fffff, K053245LgtnfghtCallback);
	K053245SetSpriteOffset(0, -368, 528);
	K05324xSetZRejection(0);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 2, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 3, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x080000, 4, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvTileRom, 0x100000);
	GfxDecode(0x8000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,  5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x080000,  6, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvSpriteRom, 0x100000);
	K053245GfxDecode(DrvSpriteRom, DrvSprites, 0x100000);

	// Load K053260 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 7, 1); if (nRet != 0) return 1;

	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x03ffff, SM_ROM);
	SekMapMemory(DrvPaletteRam       , 0x080000, 0x080fff, SM_RAM);
	SekMapMemory(Drv68KRam           , 0x090000, 0x093fff, SM_RAM);
	SekSetReadWordHandler(0, Lgtnfght68KReadWord);
	SekSetWriteWordHandler(0, Lgtnfght68KWriteWord);
	SekSetReadByteHandler(0, Lgtnfght68KReadByte);
	SekSetWriteByteHandler(0, Lgtnfght68KWriteByte);
	SekClose();

	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(LgtnfghtZ80Read);
	ZetSetWriteHandler(LgtnfghtZ80Write);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80Rom                );
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80Ram                );
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80Ram                );
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();

	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K053260Init(0, 3579545, DrvSoundRom, 0x80000);

	EEPROMInit(&thndrx2_eeprom_interface);
	
	GenericTilesInit();
	
	// Reset the driver
	SsridersDoReset();

	return 0;
}

static int Tmnt2Init()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	Tmnt2MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	Tmnt2MemIndex();
	
	K052109Init(DrvTileRom, 0x0fffff);
	K052109SetCallback(K052109TmntCallback);
	K052109AdjustScroll(0, 0);

	K053245Init(0, DrvSpriteRom, 0x3fffff, K053245SsridersCallback);
	K053245SetSpriteOffset(0, -360, 272);
	K05324xSetZRejection(0);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x040001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x040000, 3, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x080000, 6, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvTileRom, 0x100000);
	GfxDecode(0x8000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,   7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x100000,   8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x200000,   9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x300000,  10, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvSpriteRom, 0x400000);
	K053245GfxDecode(DrvSpriteRom, DrvSprites, 0x400000);
	
	// Load K053260 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 11, 1); if (nRet != 0) return 1;
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x0fffff, SM_ROM);
	SekMapMemory(Drv68KRam           , 0x104000, 0x107fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x140000, 0x140fff, SM_RAM);
	SekSetReadWordHandler(0, Ssriders68KReadWord);
	SekSetWriteWordHandler(0, Ssriders68KWriteWord);
	SekSetReadByteHandler(0, Ssriders68KReadByte);
	SekSetWriteByteHandler(0, Ssriders68KWriteByte);

	SekMapHandler(1,		  0x1c0800, 0x1c081f, SM_WRITE);
	SekSetWriteWordHandler(1, 	Tmnt268KProtWord);
	SekSetWriteByteHandler(1, 	Tmnt268KProtByte);
	SekClose();

	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(BlswhstlZ80Read);
	ZetSetWriteHandler(BlswhstlZ80Write);
	ZetMapArea(0x0000, 0xefff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0xefff, 2, DrvZ80Rom                );
	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 1, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();
	
	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K053260Init(0, 3579545, DrvSoundRom, 0x200000);

	EEPROMInit(&BlswhstlEEPROMInterface);
	
	GenericTilesInit();
	
	// Reset the driver
	SsridersDoReset();

	return 0;
}

static int QgakumonInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	Tmnt2MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	Tmnt2MemIndex();
	
	K052109Init(DrvTileRom, 0x0fffff);
	K052109SetCallback(K052109TmntCallback);
	K052109AdjustScroll(0, 0);

	K053245Init(0, DrvSpriteRom, 0x3fffff, K053245SsridersCallback);
	K053245SetSpriteOffset(0, -360, 272);
	K05324xSetZRejection(0);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x080001, 2, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x080000, 3, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x080000, 6, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvTileRom, 0x100000);
	GfxDecode(0x8000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000,   7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x200000,   8, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvSpriteRom, 0x400000);
	K053245GfxDecode(DrvSpriteRom, DrvSprites, 0x400000);
	
	// Load K053260 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000,  9, 1); if (nRet != 0) return 1;
	
	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x0fffff, SM_ROM);
	SekMapMemory(Drv68KRam           , 0x104000, 0x107fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x140000, 0x140fff, SM_RAM);
	SekSetReadWordHandler(0, Ssriders68KReadWord);
	SekSetWriteWordHandler(0, Ssriders68KWriteWord);
	SekSetReadByteHandler(0, Ssriders68KReadByte);
	SekSetWriteByteHandler(0, Ssriders68KWriteByte);

	SekMapHandler(1,		  0x1c0800, 0x1c081f, SM_WRITE);
	SekSetWriteWordHandler(1, 	Tmnt268KProtWord);
	SekSetWriteByteHandler(1, 	Tmnt268KProtByte);
	SekClose();

	// Setup the Z80 emulation
	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(BlswhstlZ80Read);
	ZetSetWriteHandler(BlswhstlZ80Write);
	ZetMapArea(0x0000, 0xefff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0xefff, 2, DrvZ80Rom                );
	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 1, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();
	
	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K053260Init(0, 3579545, DrvSoundRom, 0x200000);

	EEPROMInit(&BlswhstlEEPROMInterface);
	
	GenericTilesInit();
	
	// Reset the driver
	SsridersDoReset();

	return 0;
}

static int PunkshotInit()
{
	int nRet = 0, nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	PunkshotMemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	PunkshotMemIndex();
	
	K052109Init(DrvTileRom, 0x07ffff);
	K052109SetCallback(K052109TmntCallback);
	K052109AdjustScroll(8, 0); 

	K051960Init(DrvSpriteRom, 0x1fffff);
	K051960SetCallback(K051960PunkshotCallback);
	K051960SetSpriteOffset(8, 0);

	// Load 68000 Program Roms
	nRet = BurnLoadRom(Drv68KRom + 0x000001, 0, 2); if (nRet != 0) return 1;
	nRet = BurnLoadRom(Drv68KRom + 0x000000, 1, 2); if (nRet != 0) return 1;
	
	// Load Z80 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom, 2, 1); if (nRet != 0) return 1;

	// Load and decode the tiles
	nRet = BurnLoadRom(DrvTileRom + 0x000000, 3, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTileRom + 0x040000, 4, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvTileRom, 0x80000);
	GfxDecode(0x4000, 4, 8, 8, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x100, DrvTileRom, DrvTiles);
	
	// Load the sprites
	nRet = BurnLoadRom(DrvSpriteRom + 0x000000, 5, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvSpriteRom + 0x100000, 6, 1); if (nRet != 0) return 1;
	konami_rom_deinterleave_2(DrvSpriteRom, 0x200000);
	GfxDecode(0x4000, 4, 16, 16, SpritePlaneOffsets2, SpriteXOffsets, SpriteYOffsets, 0x400, DrvSpriteRom, DrvSprites);	

	// Load K053260 Sample Roms
	nRet = BurnLoadRom(DrvSoundRom + 0x00000, 7, 1); if (nRet != 0) return 1;

	// Setup the 68000 emulation
	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KRom           , 0x000000, 0x03ffff, SM_ROM);
	SekMapMemory(Drv68KRam  	 , 0x080000, 0x083fff, SM_RAM);
	SekMapMemory(DrvPaletteRam       , 0x090000, 0x090fff, SM_RAM);
	SekSetReadWordHandler(0, 	Punkshot68KReadWord);
	SekSetWriteWordHandler(0, 	Punkshot68KWriteWord);
	SekSetReadByteHandler(0, 	Punkshot68KReadByte);
	SekSetWriteByteHandler(0, 	Punkshot68KWriteByte);
	SekClose();

	ZetInit(1);
	ZetOpen(0);
	ZetSetReadHandler(Thndrx2Z80Read);
	ZetSetWriteHandler(Thndrx2Z80Write);
	ZetMapArea(0x0000, 0xefff, 0, DrvZ80Rom                );
	ZetMapArea(0x0000, 0xefff, 2, DrvZ80Rom                );
	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 1, DrvZ80Ram                );
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80Ram                );
	ZetMemEnd();
	ZetClose();

	// Setup the YM2151 emulation
	BurnYM2151Init(3579545, 25.0);
	
	K053260Init(0, 3579545, DrvSoundRom, 0x80000);

	GenericTilesInit();

	srand ( time(NULL) );

	// Reset the driver
	PunkshotDoReset();

	return 0;
}

static int DrvExit()
{
	SekExit();
	ZetExit();

	BurnYM2151Exit();
	UPD7759Exit();
	K007232Exit();

	GenericTilesExit();

	KonamiICExit();

	free(Mem);
	Mem = NULL;

	bIrqEnable = 0;
	DrvSoundLatch = 0;
	TitleSoundLatch = 0;
	PlayTitleSample = 0;
	TitleSamplePos = 0;
	SpriteColourBase = 0;
	memset(LayerColourBase, 0, 3);
	memset(LayerPri, 0, 3);
	PriorityFlag = 0;
	
	DrvNvRamBank = 0;
	CuebrickSndIrqFire = 0;
	BlswhstlTileRomBank = 0;
	DrvVBlank = 0;
	InitEEPROMCount = 0;

	return 0;
}

static int DrvEEPROMExit()
{
	EEPROMExit();
	
	return DrvExit();
}

static inline unsigned char pal5bit(unsigned char bits)
{
	bits &= 0x1f;
	return (bits << 3) | (bits >> 2);
}

static void DrvCalcPalette()
{
	UINT16 *PaletteRam = (UINT16*)DrvPaletteRam;
	
	for (int i = 0; i < 0x800; i += 2) {
		int Offset = i & ~1;
		UINT32 Data = (PaletteRam[Offset] << 8) | PaletteRam[Offset + 1];

		Palette[Offset >> 1] = (pal5bit(Data >> 0) << 16) | (pal5bit(Data >> 5) << 8) | pal5bit(Data >> 10);
		DrvPalette[Offset >> 1] = BurnHighCol(pal5bit(Data >> 0), pal5bit(Data >> 5), pal5bit(Data >> 10), 0);
	}
}

static void BlswhstlCalcPalette()
{
	UINT16 *PaletteRam = (UINT16*)DrvPaletteRam;
	
	for (int i = 0; i < 0x800; i++) {
		UINT32 Data = PaletteRam[i];

		Palette[i] = (pal5bit(Data >> 0) << 16) | (pal5bit(Data >> 5) << 8) | pal5bit(Data >> 10);
		DrvPalette[i] = BurnHighCol(pal5bit(Data >> 0), pal5bit(Data >> 5), pal5bit(Data >> 10), 0);
	}
}

static void TmntDraw()
{
	DrvCalcPalette();
	K052109UpdateScroll();
	
	K052109RenderLayer(2, 1, DrvTiles);
	if ((PriorityFlag & 1) == 1) K051960SpritesRender(DrvSprites, -1);
	K052109RenderLayer(1, 0, DrvTiles);
	if ((PriorityFlag & 1) == 0) K051960SpritesRender(DrvSprites, -1);
	K052109RenderLayer(0, 0, DrvTiles);
	
	BurnTransferCopy(DrvPalette);
}

static void sortlayers(int *layer,int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

static inline void BlswhstlCalcPaletteWithContrast(int i, int brt)
{
	int r,g,b;
	UINT16 *PaletteRam = (UINT16*)DrvPaletteRam;
	
	UINT32 Data = PaletteRam[i];

	r = pal5bit(Data >> 0);
	g = pal5bit(Data >> 5);
	b = pal5bit(Data >> 10);

	r = (r * brt) / 100;
	g = (g * brt) / 100;
	b = (b * brt) / 100;

	DrvPalette[i] = BurnHighCol(r, g, b, 0);
}

static void PaletteDim(int dimslayer)
{
	int i, dim, en, cb, ce, brt;

	dim = dim_v | ((~dim_c & 0x10) >> 1);
	en  = (K053251GetPriority(5) && K053251GetPriority(5) != 0x3e);

	brt = 100;
	if (en) brt -= 40*dim/8;

	if (brt < 100) {
		cb = LayerColourBase[dimslayer] << 4;
		ce = cb + 128;

		for (i =  0; i < cb; i++)
			BlswhstlCalcPaletteWithContrast(i, brt);

		for (i = cb; i < ce; i++) // text
			BlswhstlCalcPaletteWithContrast(i, 100);

		for (i = ce; i < 2048; i++)
			BlswhstlCalcPaletteWithContrast(i, brt);
	} else {
		BlswhstlCalcPalette();
	}
}

static void BlswhstlDraw()
{
	int Layer[3];
	
	K052109UpdateScroll();
	
	int BGColourBase   = K053251GetPaletteIndex(0);
	SpriteColourBase   = K053251GetPaletteIndex(1);
	LayerColourBase[0] = K053251GetPaletteIndex(2);
	LayerColourBase[1] = K053251GetPaletteIndex(4);
	LayerColourBase[2] = K053251GetPaletteIndex(3);
	
	LayerPri[0] = K053251GetPriority(2);
	LayerPri[1] = K053251GetPriority(4);
	LayerPri[2] = K053251GetPriority(3);
	Layer[0] = 0;
	Layer[1] = 1;
	Layer[2] = 2;
	
	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
		pTransDraw[i] = 16 * BGColourBase;
	}

	sortlayers(Layer, LayerPri);

	PaletteDim(Layer[2]);

	K053245SpritesRender(0, DrvSprites, 3);
	if (nBurnLayer & 1) K052109RenderLayer(Layer[0], 0, DrvTiles);
	K053245SpritesRender(0, DrvSprites, 2);
	if (nBurnLayer & 2) K052109RenderLayer(Layer[1], 0, DrvTiles);
	K053245SpritesRender(0, DrvSprites, 1);
	K053245SpritesRender(0, DrvSprites, 0);
	if (nBurnLayer & 4) K052109RenderLayer(Layer[2], 0, DrvTiles);	

	KonamiBlendCopy(Palette, DrvPalette);
}

static int Thndrx2Draw()
{
	int Layer[3];
	
	BlswhstlCalcPalette();
	K052109UpdateScroll();
	
	int BGColourBase   = K053251GetPaletteIndex(0);
	SpriteColourBase   = K053251GetPaletteIndex(1);
	LayerColourBase[0] = K053251GetPaletteIndex(2);
	LayerColourBase[1] = K053251GetPaletteIndex(4);
	LayerColourBase[2] = K053251GetPaletteIndex(3);
	
	LayerPri[0] = K053251GetPriority(2);
	LayerPri[1] = K053251GetPriority(4);
	LayerPri[2] = K053251GetPriority(3);
	Layer[0] = 0;
	Layer[1] = 1;
	Layer[2] = 2;
	
	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
		pTransDraw[i] = 16 * BGColourBase;
	}

	sortlayers(Layer, LayerPri);

	if (nSpriteEnable & 8) K051960SpritesRender(DrvSprites, 3);
	if (nBurnLayer & 1) K052109RenderLayer(Layer[0], 0, DrvTiles);
	if (nSpriteEnable & 4) K051960SpritesRender(DrvSprites, 2);	
	if (nBurnLayer & 2) K052109RenderLayer(Layer[1], 0, DrvTiles);
	if (nSpriteEnable & 2) K051960SpritesRender(DrvSprites, 1);
	if (nSpriteEnable & 1) K051960SpritesRender(DrvSprites, 0);
	if (nBurnLayer & 4) K052109RenderLayer(Layer[2], 0, DrvTiles);

	BurnTransferCopy(DrvPalette);

	return 0;
}

static int PunkshotDraw()
{
	int Layer[3];
	
	BlswhstlCalcPalette();
	K052109UpdateScroll();
	
//	int BGColourBase   = K053251GetPaletteIndex(0);
	SpriteColourBase   = K053251GetPaletteIndex(1);
	LayerColourBase[0] = K053251GetPaletteIndex(2);
	LayerColourBase[1] = K053251GetPaletteIndex(4);
	LayerColourBase[2] = K053251GetPaletteIndex(3);
	
	LayerPri[0] = K053251GetPriority(2);
	LayerPri[1] = K053251GetPriority(4);
	LayerPri[2] = K053251GetPriority(3);
	Layer[0] = 0;
	Layer[1] = 1;
	Layer[2] = 2;

	sortlayers(Layer, LayerPri);

//	if (nSpriteEnable & 8) K051960SpritesRender(DrvSprites, 3); // not sure...
	if (nBurnLayer & 1) K052109RenderLayer(Layer[0], 1, DrvTiles);
	if (nSpriteEnable & 4) K051960SpritesRender(DrvSprites, 2);	
	if (nBurnLayer & 2) K052109RenderLayer(Layer[1], 0, DrvTiles);
	if (nSpriteEnable & 2) K051960SpritesRender(DrvSprites, 1);
	if (nBurnLayer & 4) K052109RenderLayer(Layer[2], 0, DrvTiles);
	if (nSpriteEnable & 1) K051960SpritesRender(DrvSprites, 0);

	BurnTransferCopy(DrvPalette);

	return 0;
}

static void RenderTitleSample(short *pSoundBuf, int nLength)
{
	double Addr = TitleSamplePos;
	double Step = (double)20000 / nBurnSoundRate;
	
	for (int i = 0; i < nLength; i += 2) {
		if (Addr > 0x3ffff) break;
		INT16 Sample = DrvTitleSample[(int)Addr];
		
		pSoundBuf[i + 0] += Sample;
		pSoundBuf[i + 1] += Sample;
		
		Addr += Step;
	}
	
	TitleSamplePos = Addr;
}

static int TmntFrame()
{
	int nInterleave = nBurnSoundLen;
	int nSoundBufferPos = 0;

	if (DrvReset) TmntDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 8000000 / 60;
	nCyclesTotal[1] = 3579545 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		SekOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		if (i == (nInterleave - 1) && bIrqEnable) SekSetIRQLine(5, SEK_IRQSTATUS_AUTO);
		SekClose();
		
		// Run Z80
		nCurrentCPU = 1;
		ZetOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		ZetClose();
		
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K007232Update(0, pSoundBuf, nSegmentLength);
			UPD7759Update(0, pSoundBuf, nSegmentLength);
			if (PlayTitleSample) RenderTitleSample(pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K007232Update(0, pSoundBuf, nSegmentLength);
			UPD7759Update(0, pSoundBuf, nSegmentLength);
			if (PlayTitleSample) RenderTitleSample(pSoundBuf, nSegmentLength);
		}
	}
	
	if (pBurnDraw) TmntDraw();

	return 0;
}

static int MiaFrame()
{
	int nInterleave = nBurnSoundLen;
	int nSoundBufferPos = 0;

	if (DrvReset) DrvDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 8000000 / 60;
	nCyclesTotal[1] = 3579545 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		SekOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		if (i == (nInterleave - 1) && bIrqEnable) SekSetIRQLine(5, SEK_IRQSTATUS_AUTO);
		SekClose();
		
		// Run Z80
		nCurrentCPU = 1;
		ZetOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		ZetClose();
		
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K007232Update(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K007232Update(0, pSoundBuf, nSegmentLength);
		}
	}
	
	if (pBurnDraw) TmntDraw();

	return 0;
}

static int CuebrickFrame()
{
	int nInterleave = 10;
	int nSoundBufferPos = 0;

	if (DrvReset) CuebrickDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 8000000 / 60;
	nCyclesDone[0] = 0;

	SekNewFrame();
	SekOpen(0);
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;		
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		if (i == (nInterleave - 1) && bIrqEnable) SekSetIRQLine(5, SEK_IRQSTATUS_AUTO);
		if (CuebrickSndIrqFire) SekSetIRQLine(6, SEK_IRQSTATUS_AUTO);
				
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
		}
	}
	
	SekClose();
	
	if (pBurnDraw) TmntDraw();

	return 0;
}

static int BlswhstlFrame()
{
	int nInterleave = 32;
	int nSoundBufferPos = 0;

	if (DrvReset) BlswhstlDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 16000000 / 60;
	nCyclesTotal[1] = 3579545 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		SekOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		if (i == (nInterleave - 1) && K052109_irq_enabled) SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);
		SekClose();
		
		// Run Z80
		nCurrentCPU = 1;
		ZetOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		ZetClose();
		
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
		}
	}
	
	if (pBurnDraw) BlswhstlDraw();
	
	K053245ClearBuffer(0);

	return 0;
}

static int SsridersFrame()
{
	int nInterleave = 262;
	int nSoundBufferPos = 0;

	if (DrvReset) SsridersDoReset();

	SsridersMakeInputs();

	nCyclesTotal[0] = 16000000 / 60;
	nCyclesTotal[1] = 4000000 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		SekOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		if (i == 19) DrvVBlank = 0;
		if (i == 243) DrvVBlank = 1;
		if (i == 243 && K052109_irq_enabled) SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);
		SekClose();
		
		// Run Z80
		nCurrentCPU = 1;
		ZetOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		ZetClose();
		
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
		}
	}
	
	if (pBurnDraw) BlswhstlDraw();

	return 0;
}

static int Thndrx2Frame()
{
	int nInterleave = 100;
	int nSoundBufferPos = 0;

	if (DrvReset) SsridersDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 12000000 / 60;
	nCyclesTotal[1] = 3579545 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	SekOpen(0);
	ZetOpen(0);

	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		
		// Run Z80
		nCurrentCPU = 1;
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;

		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	if (K052109_irq_enabled) SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);

	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
		}
	}

	ZetClose();
	SekClose();

	if (pBurnDraw) Thndrx2Draw();

	return 0;
}

static int LgtnfghtFrame()
{
	int nInterleave = 100;
	int nSoundBufferPos = 0;

	if (DrvReset) SsridersDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 12000000 / 60;
	nCyclesTotal[1] = 3579545 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	SekOpen(0);
	ZetOpen(0);

	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		
		// Run Z80
		nCurrentCPU = 1;
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;

		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	if (K052109_irq_enabled) SekSetIRQLine(5, SEK_IRQSTATUS_AUTO);

	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
		}
	}

	ZetClose();
	SekClose();

	if (pBurnDraw) BlswhstlDraw();

	return 0;
}

static int Tmnt2Frame()
{
	int nInterleave = 262;
	int nSoundBufferPos = 0;

	if (DrvReset) SsridersDoReset();

	SsridersMakeInputs();

	nCyclesTotal[0] = 16000000 / 60;
	nCyclesTotal[1] = 8000000 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		SekOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		if (i == 19) DrvVBlank = 0;
		if (i == 243) DrvVBlank = 1;
		if (i == 243 && K052109_irq_enabled) SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);
		SekClose();
		
		// Run Z80
		nCurrentCPU = 1;
		ZetOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		ZetClose();
		
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
		}
	}
	
	if (pBurnDraw) BlswhstlDraw();

	return 0;
}

static int PunkshotFrame()
{
	int nInterleave = 256;
	int nSoundBufferPos = 0;

	if (DrvReset) PunkshotDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = 12000000 / 60;
	nCyclesTotal[1] = 3579545 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	SekNewFrame();
	ZetNewFrame();
	
	SekOpen(0);
	ZetOpen(0);

	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run 68000
		nCurrentCPU = 0;
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		
		// Run Z80
		nCurrentCPU = 1;
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;

		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	if (K052109_irq_enabled) SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);

	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			K053260Update(0, pSoundBuf, nSegmentLength);
		}
	}

	ZetClose();
	SekClose();

	if (pBurnDraw) PunkshotDraw();

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {			// Return minimum compatible version
		*pnMin = 0x029693;
	}

	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = RamStart;
		ba.nLen	  = RamEnd-RamStart;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}
	
	KonamiICScan(nAction);
	
	if (nAction & ACB_DRIVER_DATA) {
		SekScan(nAction);
		
		BurnYM2151Scan(nAction);		

		// Scan critical driver variables
		SCAN_VAR(nCyclesDone);
		SCAN_VAR(nCyclesSegment);
		SCAN_VAR(DrvDip);
		SCAN_VAR(DrvInput);
		SCAN_VAR(bIrqEnable);
		SCAN_VAR(DrvSoundLatch);
		SCAN_VAR(TitleSoundLatch);
		SCAN_VAR(PlayTitleSample);
		SCAN_VAR(TitleSamplePos);
		SCAN_VAR(PriorityFlag);
	}

	return 0;
}

static int TmntScan(int nAction, int *pnMin)
{
	if (nAction & ACB_DRIVER_DATA) {
		ZetScan(nAction);
		K007232Scan(nAction, pnMin);
		UPD7759Scan(0, nAction, pnMin);
	}
	
	return DrvScan(nAction, pnMin);
}

static int MiaScan(int nAction, int *pnMin)
{
	if (nAction & ACB_DRIVER_DATA) {
		ZetScan(nAction);
		K007232Scan(nAction, pnMin);
	}
	
	return DrvScan(nAction, pnMin);
}

static int CuebrickScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (nAction & ACB_NVRAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data = DrvNvRam;
		ba.nLen = 0x400 * 0x20;
		ba.szName = "NV RAM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		KonamiICScan(nAction);
		SCAN_VAR(CuebrickSndIrqFire);
	}

	return DrvScan(nAction, pnMin);
}

static int BlswhstlScan(int nAction, int *pnMin)
{
	if (nAction & ACB_DRIVER_DATA) {
		ZetScan(nAction);
		K053260Scan(nAction);
				
		SCAN_VAR(InitEEPROMCount);
	}
	
	EEPROMScan(nAction, pnMin);
	
	return DrvScan(nAction, pnMin);
}

static int SsridersScan(int nAction, int *pnMin)
{
	if (nAction & ACB_DRIVER_DATA) {
		ZetScan(nAction);
		K053260Scan(nAction);
				
		SCAN_VAR(InitEEPROMCount);
		SCAN_VAR(DrvVBlank);
	}
	
	EEPROMScan(nAction, pnMin);
	
	return DrvScan(nAction, pnMin);
}

static int Thndrx2aScan(int nAction, int *pnMin)
{
	if (nAction & ACB_DRIVER_DATA) {
		ZetScan(nAction);
		K053260Scan(nAction);
	}
	
	return DrvScan(nAction, pnMin);
}

struct BurnDriver BurnDrvTmnt = {
	"tmnt", NULL, NULL, "1989",
	"Teenage Mutant Ninja Turtles (World 4 Players)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 4, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, TmntRomInfo, TmntRomName, TmntInputInfo, TmntDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmntu = {
	"tmntu", "tmnt", NULL, "1989",
	"Teenage Mutant Ninja Turtles (US 4 Players, set 1)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, TmntuRomInfo, TmntuRomName, TmntInputInfo, TmntDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmntua = {
	"tmntua", "tmnt", NULL, "1989",
	"Teenage Mutant Ninja Turtles (US 4 Players, set 2)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, TmntuaRomInfo, TmntuaRomName, TmntInputInfo, TmntDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmht = {
	"tmht", "tmnt", NULL, "1989",
	"Teenage Mutant Hero Turtles (UK 4 Players)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, TmhtRomInfo, TmhtRomName, TmntInputInfo, TmntDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmntj = {
	"tmntj", "tmnt", NULL, "1990",
	"Teenage Mutant Ninja Turtles (Japan 4 Players)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, TmntjRomInfo, TmntjRomName, TmntInputInfo, TmntDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmht2p = {
	"tmht2p", "tmnt", NULL, "1989",
	"Teenage Mutant Hero Turtles (UK 2 Players, set 1)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, Tmht2pRomInfo, Tmht2pRomName, Tmnt2pInputInfo, Tmnt2pDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmht2pa = {
	"tmht2pa", "tmnt", NULL, "1989",
	"Teenage Mutant Hero Turtles (UK 2 Players, set 2)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, Tmht2paRomInfo, Tmht2paRomName, Tmnt2pInputInfo, Tmnt2pDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmht2pj = {
	"tmnt2pj", "tmnt", NULL, "1990",
	"Teenage Mutant Ninja Turtles (Japan 2 Players)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, Tmnt2pjRomInfo, Tmnt2pjRomName, Tmnt2pInputInfo, Tmnt2pDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmht2po = {
	"tmnt2po", "tmnt", NULL, "1989",
	"Teenage Mutant Ninja Turtles (Oceania 2 Players)\0", NULL, "Konami", "GX963",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, Tmnt2poRomInfo, Tmnt2poRomName, Tmnt2pInputInfo, Tmnt2pDIPInfo,
	TmntInit, DrvExit, TmntFrame, NULL, TmntScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvMia = {
	"mia", NULL, NULL, "1989",
	"M.I.A. - Missing in Action (version T)\0", NULL, "Konami", "GX808",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, MiaRomInfo, MiaRomName, MiaInputInfo, MiaDIPInfo,
	MiaInit, DrvExit, MiaFrame, NULL, MiaScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvMia2 = {
	"mia2", "mia", NULL, "1989",
	"M.I.A. - Missing in Action (version S)\0", NULL, "Konami", "GX808",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, Mia2RomInfo, Mia2RomName, MiaInputInfo, MiaDIPInfo,
	MiaInit, DrvExit, MiaFrame, NULL, MiaScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvCuebrick = {
	"cuebrick", NULL, NULL, "1989",
	"Cue Brick (World version D)\0", NULL, "Konami", "GX903",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, CuebrickRomInfo, CuebrickRomName, MiaInputInfo, MiaDIPInfo,
	CuebrickInit, DrvExit, CuebrickFrame, NULL, CuebrickScan,
	0, NULL, NULL, NULL, NULL, 0x400, 304, 224, 4, 3
};

struct BurnDriver BurnDrvBlswhstl = {
	"blswhstl", NULL, NULL, "1991",
	"Bells & Whistles (Version L)\0", NULL, "Konami", "GX060",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_KONAMI_68K_Z80, GBF_VERSHOOT, 0,
	NULL, BlswhstlRomInfo, BlswhstlRomName, BlswhstlInputInfo, NULL,
	BlswhstlInit, DrvEEPROMExit, BlswhstlFrame, NULL, BlswhstlScan,
	0, NULL, NULL, NULL, NULL, 0x810, 224, 288, 3, 4
};

struct BurnDriver BurnDrvDetatwin = {
	"detatwin", "blswhstl", NULL, "1991",
	"Detana!! Twin Bee (Japan ver. J)\0", NULL, "Konami", "GX060",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_KONAMI_68K_Z80, GBF_VERSHOOT, 0,
	NULL, DetatwinRomInfo, DetatwinRomName, BlswhstlInputInfo, NULL,
	BlswhstlInit, DrvEEPROMExit, BlswhstlFrame, NULL, BlswhstlScan,
	0, NULL, NULL, NULL, NULL, 0x810, 224, 288, 3, 4
};

struct BurnDriver BurnDrvSsriders = {
	"ssriders", NULL, NULL, "1991",
	"Sunset Riders (4 Players ver EAC)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 4, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsridersRomInfo, SsridersRomName, Ssriders4pInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdrebd = {
	"ssridersebd", "ssriders", NULL, "1991",
	"Sunset Riders (2 Players ver EBD)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdrebdRomInfo, SsrdrebdRomName, SsridersInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdrebc = {
	"ssridersebc", "ssriders", NULL, "1991",
	"Sunset Riders (2 Players ver EBC)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdrebcRomInfo, SsrdrebcRomName, SsridersInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdruda = {
	"ssridersuda", "ssriders", NULL, "1991",
	"Sunset Riders (4 Players ver UDA)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdrudaRomInfo, SsrdrudaRomName, Ssriders4psInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdreaa = {
	"ssriderseaa", "ssriders", NULL, "1991",
	"Sunset Riders (4 Players ver EAA)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdreaaRomInfo, SsrdreaaRomName, Ssriders4psInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdruac = {
	"ssridersuac", "ssriders", NULL, "1991",
	"Sunset Riders (4 Players ver UAC)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdruacRomInfo, SsrdruacRomName, Ssriders4pInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdrubc = {
	"ssridersubc", "ssriders", NULL, "1991",
	"Sunset Riders (2 Players ver UBC)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdrubcRomInfo, SsrdrubcRomName, SsridersInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdrabd = {
	"ssridersabd", "ssriders", NULL, "1991",
	"Sunset Riders (2 Players ver ABD)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdrabdRomInfo, SsrdrabdRomName, SsridersInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdradd = {
	"ssridersadd", "ssriders", NULL, "1991",
	"Sunset Riders (4 Players ver ADD)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 4, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdraddRomInfo, SsrdraddRomName, Ssriders4psInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvSsrdrjbd = {
	"ssridersjbd", "ssriders", NULL, "1991",
	"Sunset Riders (2 Players ver JBD)\0", NULL, "Konami", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, SsrdrjbdRomInfo, SsrdrjbdRomName, SsridersInputInfo, NULL,
	SsridersInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

static int SunsetblInit()
{
	return 1;
}

struct BurnDriver BurnDrvSunsetbl = {
	"ssridersb", "ssriders", NULL, "1991",
	"Sunset Riders (bootleg 4 Players ver ADD)\0", NULL, "Bootleg", "GX064",
	NULL, NULL, NULL, NULL,
	BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_PLATFORM, 0,
	NULL, sunsetblRomInfo, sunsetblRomName, Ssriders4psInputInfo, NULL,
	SunsetblInit, DrvEEPROMExit, SsridersFrame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x810, 288, 224, 4, 3
};

struct BurnDriver BurnDrvThndrx2 = {
	"thndrx2", NULL, NULL, "1991",
	"Thunder Cross II (World)\0", NULL, "Konami", "GX073",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_KONAMI_68K_Z80, GBF_HORSHOOT, 0,
	NULL, thndrx2RomInfo, thndrx2RomName, Thndrx2InputInfo, NULL,
	Thndrx2Init, DrvEEPROMExit, Thndrx2Frame, NULL, Thndrx2aScan,
	0, NULL, NULL, NULL, NULL, 0x800, 288, 224, 4, 3
};

struct BurnDriver BurnDrvThndrx2a = {
	"thndrx2a", "thndrx2", NULL, "1991",
	"Thunder Cross II (Asia)\0", NULL, "Konami", "GX073",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_HORSHOOT, 0,
	NULL, thndrx2aRomInfo, thndrx2aRomName, Thndrx2InputInfo, NULL,
	Thndrx2Init, DrvEEPROMExit, Thndrx2Frame, NULL, Thndrx2aScan,
	0, NULL, NULL, NULL, NULL, 0x800, 288, 224, 4, 3
};

struct BurnDriver BurnDrvThndrx2j = {
	"thndrx2j", "thndrx2", NULL, "1991",
	"Thunder Cross II (Japan)\0", NULL, "Konami", "GX073",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_HORSHOOT, 0,
	NULL, thndrx2jRomInfo, thndrx2jRomName, Thndrx2InputInfo, NULL,
	Thndrx2Init, DrvEEPROMExit, Thndrx2Frame, NULL, Thndrx2aScan,
	0, NULL, NULL, NULL, NULL, 0x800, 288, 224, 4, 3
};

struct BurnDriver BurnDrvLgtnfght = {
	"lgtnfght", NULL, NULL, "1990",
	"Lightning Fighters (World)\0", NULL, "Konami", "GX939",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_KONAMI_68K_Z80, GBF_VERSHOOT, 0,
	NULL, lgtnfghtRomInfo, lgtnfghtRomName, LgtnfghtInputInfo, LgtnfghtDIPInfo,
	LgtnfghtInit, DrvExit, LgtnfghtFrame, NULL, Thndrx2aScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 224, 288, 3, 4
};

struct BurnDriver BurnDrvLgtnfghu = {
	"lgtnfghtu", "lgtnfght", NULL, "1990",
	"Lightning Fighters (US)\0", NULL, "Konami", "GX939",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_KONAMI_68K_Z80, GBF_VERSHOOT, 0,
	NULL, lgtnfghuRomInfo, lgtnfghuRomName, LgtnfghtInputInfo, LgtnfghtDIPInfo,
	LgtnfghtInit, DrvExit, LgtnfghtFrame, NULL, Thndrx2aScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 224, 288, 3, 4
};

struct BurnDriver BurnDrvLgtnfgha = {
	"lgtnfghta", "lgtnfght", NULL, "1990",
	"Lightning Fighters (Asia)\0", NULL, "Konami", "GX939",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_KONAMI_68K_Z80, GBF_VERSHOOT, 0,
	NULL, lgtnfghaRomInfo, lgtnfghaRomName, LgtnfghtInputInfo, LgtnfghtDIPInfo,
	LgtnfghtInit, DrvExit, LgtnfghtFrame, NULL, Thndrx2aScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 224, 288, 3, 4
};

struct BurnDriver BurnDrvTrigon = {
	"trigon", "lgtnfght", NULL, "1990",
	"Trigon (Japan)\0", NULL, "Konami", "GX939",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_KONAMI_68K_Z80, GBF_VERSHOOT, 0,
	NULL, trigonRomInfo, trigonRomName, LgtnfghtInputInfo, LgtnfghtDIPInfo,
	LgtnfghtInit, DrvExit, LgtnfghtFrame, NULL, Thndrx2aScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 224, 288, 3, 4
};

struct BurnDriver BurnDrvTmnt2 = {
	"tmnt2", NULL, NULL, "1991",
	"Teenage Mutant Ninja Turtles - Turtles in Time (4 Players ver UAA)\0", NULL, "Konami", "GX063",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, tmnt2RomInfo, tmnt2RomName, Ssriders4pInputInfo, NULL,
	Tmnt2Init, DrvEEPROMExit, Tmnt2Frame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x800, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmnt22pu = {
	"tmnt22pu", "tmnt2", NULL, "1991",
	"Teenage Mutant Ninja Turtles - Turtles in Time (2 Players ver UDA)\0", NULL, "Konami", "GX063",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, tmnt22puRomInfo, tmnt22puRomName, SsridersInputInfo, NULL,
	Tmnt2Init, DrvEEPROMExit, Tmnt2Frame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x800, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmht22pe = {
	"tmht22pe", "tmnt2", NULL, "1991",
	"Teenage Mutant Hero Turtles - Turtles in Time (2 Players ver EBA)\0", NULL, "Konami", "GX063",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, tmht22peRomInfo, tmht22peRomName, SsridersInputInfo, NULL,
	Tmnt2Init, DrvEEPROMExit, Tmnt2Frame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x800, 304, 224, 4, 3
};

struct BurnDriver BurnDrvTmnt2a = {
	"tmnt2a", "tmnt2", NULL, "1991",
	"Teenage Mutant Ninja Turtles - Turtles in Time (4 Players ver ADA)\0", NULL, "Konami", "GX063",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, tmnt2aRomInfo, tmnt2aRomName, Ssriders4psInputInfo, NULL,
	Tmnt2Init, DrvEEPROMExit, Tmnt2Frame, NULL, SsridersScan,
	0, NULL, NULL, NULL, NULL, 0x800, 304, 224, 4, 3
};

struct BurnDriver BurnDrvQgakumon = {
	"qgakumon", NULL, NULL, "1993",
	"Quiz Gakumon no Susume (Japan ver. JA2 Type L)\0", NULL, "Konami", "GX248",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_KONAMI_68K_Z80, GBF_QUIZ, 0,
	NULL, qgakumonRomInfo, qgakumonRomName, QgakumonInputInfo, NULL,
	QgakumonInit, DrvEEPROMExit, Tmnt2Frame, NULL, SsridersScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 304, 224, 4, 3
};

struct BurnDriver BurnDrvPunkshot = {
	"punkshot", NULL, NULL, "1990",
	"Punk Shot (US 4 Players)\0", NULL, "Konami", "GX907",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_KONAMI_68K_Z80, GBF_SPORTSMISC, 0,
	NULL, punkshotRomInfo, punkshotRomName, PunkshotInputInfo, PunkshotDIPInfo,
	PunkshotInit, DrvExit, PunkshotFrame, NULL, Thndrx2aScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 288, 224, 4, 3
};

struct BurnDriver BurnDrvPunksht2 = {
	"punkshot2", "punkshot", NULL, "1990",
	"Punk Shot (US 2 Players)\0", NULL, "Konami", "GX907",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SPORTSMISC, 0,
	NULL, punksht2RomInfo, punksht2RomName, Punksht2InputInfo, Punksht2DIPInfo,
	PunkshotInit, DrvExit, PunkshotFrame, NULL, Thndrx2aScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 288, 224, 4, 3
};

struct BurnDriver BurnDrvPunkshtj = {
	"punkshotj", "punkshot", NULL, "1990",
	"Punk Shot (Japan 2 Players)\0", NULL, "Konami", "GX907",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_KONAMI_68K_Z80, GBF_SPORTSMISC, 0,
	NULL, punkshtjRomInfo, punkshtjRomName, Punksht2InputInfo, Punksht2DIPInfo,
	PunkshotInit, DrvExit, PunkshotFrame, NULL, Thndrx2aScan, 
	0, NULL, NULL, NULL, NULL, 0x800, 288, 224, 4, 3
};
