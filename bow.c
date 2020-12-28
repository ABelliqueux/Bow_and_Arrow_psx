// Bow & Arrow PSX, by Schnappy, 12-2020
// V0.1
//
// Base on the original dos game by John Di Troia, From Bow and Arrow - In Search of the Greatest Archer v1.0 (1992)
// https://www.classicdosgames.com/company/johnditroia.html
//
// WARNING : Following code is *TERRIBLE*, I am learning C, thus the terrible quality. But it does the job ¯\_(ツ)_/¯
// 

#include <sys/types.h>
#include <stdio.h>
#include <libgte.h>
#include <libetc.h>
#include <libgpu.h>
#include <kernel.h>
#include <rand.h>

// VAG playback

#include <libsnd.h>
#include <libspu.h>

#include "animation.c"


typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int    DWORD;

// end VAG playback

#define VMODE 0         // Video Mode : 0 : NTSC, 1: PAL

#define SCREENXRES 320
#define SCREENYRES 240

#define CENTERX SCREENXRES/2
#define CENTERY SCREENYRES/2

#define START_POS CENTERX - 20
    
#define MARGINX 32               // margins for text display
#define MARGINY 8

#define FONTSIZE 8 * 6           // Text Field Height

#define OTLEN 64                 // Ordering Table Length 

// animation related defines

#define SPRITESHEETGRIDXSCALE 42        // one frame every 42 pixels
#define SPRITESHEETGRIDYSCALE 48 

#define ARROW_SPEED 3

// Graphics stuff

DISPENV disp[2];                    // Double buffered DISPENV and DRAWENV
DRAWENV draw[2];

u_long ot[2][OTLEN];                // double ordering table of length 8 * 32 = 256 bits / 32 bytes

char primbuff[2][65536] = {0};      // double primitive buffer of length 32768 * 8 =  262.144 bits / 32,768 Kbytes

char *nextpri = primbuff[0];        // pointer to the next primitive in primbuff. Initially, points to the first bit of primbuff[0]

short db = 0;                       // index of which buffer is used, values 0, 1

// SOUND STUFF

// VAG playback
#define MALLOC_MAX 16

// convert Little endian to Big endian
#define SWAP_ENDIAN32(x) (((x)>>24) | (((x)>>8) & 0xFF00) | (((x)<<8) & 0x00FF0000) | ((x)<<24)) 

typedef struct VAGheader{		// All the values in this header must be big endian
        char id[4];			    // VAGp         4 bytes -> 1 char * 4
        DWORD version;          // 4 bytes
        DWORD reserved;         // 4 bytes
        DWORD dataSize;         // (in bytes) 4 bytes
        DWORD samplingFrequency;// 4 bytes
        char  reserved2[12];    // 12 bytes -> 1 char * 12
        char  name[16];         // 16 bytes -> 1 char * 16
        // Waveform data after that
}VAGhdr;

SpuCommonAttr commonAttributes;         // structure for changing common voice attributes
SpuVoiceAttr  voiceAttributes ;         // structure for changing individual voice attributes


// address allocated in memory for first sound file
u_long vag_spu_address;                  // music
u_long vag1_spu_address;                 // pop
u_long vag2_spu_address;                 // ready
u_long vag3_spu_address;                 // shoot
u_long vag4_spu_address;                 // jump
u_long vag5_spu_address;                 // gameover
u_long vag6_spu_address;                 // gameover
u_long vag7_spu_address;                 // gameover


// SOUND DEBUG : these allow printing values for debugging

u_long spu_start_address;                
u_long spu1_start_address;              
u_long spu2_start_address;              
u_long spu3_start_address;              
u_long spu4_start_address;              
u_long spu5_start_address;              
u_long spu6_start_address;              
u_long spu7_start_address;              

u_long get_start_addr;
u_long get1_start_addr;
u_long get2_start_addr;
u_long get3_start_addr;
u_long get4_start_addr;
u_long get5_start_addr;
u_long get6_start_addr;
u_long get7_start_addr;

u_long transSize;                            
u_long trans1Size;    
u_long trans2Size;    
u_long trans3Size;    
u_long trans4Size;    
u_long trans5Size;    
u_long trans6Size;    
u_long trans7Size;    

char spu_malloc_rec[SPU_MALLOC_RECSIZ * (2 + MALLOC_MAX+1)]; // Memory management table ; allow MALLOC_MAX calls to SpuMalloc() - ibref47.pdf p.1044

// VAG files

// We're using GrumpyCoder's Nugget wrapper to compile the code with a modern GCC : https://github.com/grumpycoders/pcsx-redux/tree/main/src/mips/psyq
// To include binary files in the exe, add your VAG files to the SRCS variable in Makefile
// and in common.mk, add this rule to include *.vag files :
//
//~ %.o: %.vag
	//~ $(PREFIX)-objcopy -I binary --set-section-alignment .data=4 --rename-section .data=.rodata,alloc,load,readonly,data,contents -O elf32-tradlittlemips -B mips $< $@


// vil.vag - 22050 Khz
extern BYTE _binary_VAG_vil_vag_start[]; // filename must begin with _binary_ followed by the full path, with . and / replaced, and then suffixed with _ and end with _start[]; or end[];
extern BYTE _binary_VAG_vil_vag_end[];   // https://discord.com/channels/642647820683444236/663664210525290507/780866265077383189

// ready.vag - 44100 Khz
extern BYTE _binary_VAG_ready_vag_start[];
extern BYTE _binary_VAG_ready_vag_end[];

// shoot.vag - 44100 Khz
extern BYTE _binary_VAG_shoot_vag_start[];
extern BYTE _binary_VAG_shoot_vag_end[];

// pop.vag - 44100 Khz
extern BYTE _binary_VAG_pop_vag_start[];
extern BYTE _binary_VAG_pop_vag_end[];

// jmp.vag - 44100 Khz
extern BYTE _binary_VAG_jmp_vag_start[];
extern BYTE _binary_VAG_jmp_vag_end[];

// toc.vag - 44100 Khz
extern BYTE _binary_VAG_toc_vag_start[];
extern BYTE _binary_VAG_toc_vag_end[];

// crowd.vag - 44100 Khz
extern BYTE _binary_VAG_crowd_vag_start[];
extern BYTE _binary_VAG_crowd_vag_end[];

// folks.vag - 44100 Khz
extern BYTE _binary_VAG_folks_vag_start[];
extern BYTE _binary_VAG_folks_vag_end[];


// define which SFX play on which voice
#define MUSIC SPU_0CH
#define READY SPU_2CH
#define SHOOT SPU_3CH
#define POP   SPU_4CH
#define JMP   SPU_5CH
#define CROWD SPU_6CH
#define FOLKS SPU_7CH
#define TOC   SPU_8CH

// end VAG playback

// game stuff 

char gameStarted = 0;
char gameReset   = 0;
char intro = 1;
short nbrBalloonsHit = 0;
short nbrBubblesHit = 0;
char hit = 0;


//animation player

u_long globalTime = 0;
u_long lastFrameTime = 0;

frame animPlayerQueue[128]; // 128 x 4 bytes buffer to store animation frames : 1 frame == 14 bytes
frame *nextFrame = animPlayerQueue;
short animPlayerQueueSize;

// pad stuff

char buttonRActive = 0;
char buttonUActive = 0;
char buttonDActive = 0;
char buttonLActive = 0;

char buttonTimer = 0;

char btnUp    = 0;
char btnRight = 0;
char btnDown  = 0;
char btnLeft  = 0;

char arrowReleased = 0;
char iddleRobin = 1;

// nugget - import binary, see makefile also
extern ulong _binary_TIM_bowsht_tim_start[]; // filename must begin with _binary_ followed by the full path, with . and / replaced, and then suffixed with _ and end with _start[]; or end[]; https://discord.com/channels/642647820683444236/663664210525290507/780866265077383189
extern ulong _binary_TIM_bowsht_tim_end[];   // _start and _end are pointers, while _length is a constant.

extern ulong _binary_TIM_bowbg_tim_start[];
extern ulong _binary_TIM_bowbg_tim_end[];

extern ulong _binary_TIM_menu_tim_start[];
extern ulong _binary_TIM_menu_tim_end[];

extern ulong _binary_TIM_inst_tim_start[];
extern ulong _binary_TIM_inst_tim_end[];



int bowsht_mode, bowbg_mode, menu_mode, inst_mode;
RECT bowsht_prect, bowsht_crect, bowbg_prect, bowbg_crect, menu_prect, menu_crect, inst_prect, inst_crect ;

CVECTOR BgColor = {104,169,16};

typedef struct {
    short x, y;
    short r;
    } CRCL;

typedef struct
    { 
    POLY_FT4 * poly_ft4;
    CVECTOR   color;
    short     width;
    short     height;
    //~ VECTOR    PosV_L; // Not used anymore
    SVECTOR   RotV_L;
    VECTOR    TransV_L;
    VECTOR    ScaleV_L;
    SVECTOR   PivotV_L;
    SVECTOR   Verts[4];
    MATRIX    Matrix;
    long      depth;
    long      flag;
    short     rotSpeed;
    int       otz;
    short     hit;
    short     cursor;
    short     timer;
    frame     *anim;
    } polygon;
    
typedef struct {
    short ammo;
    short nbrBalloons;
    short nbrBubbles;
    short nbrTarget;
    short nbrArrows;
    } ASSETS;
    
typedef struct{
    short x, y;
    } COORD;

ASSETS assetsLoad  = {24, 10, 8, 1, 5};

short currentLvl = 0;

/////

int sync;

void display(void)
{
    DrawSync(0);
    sync = VSync(0);
    
    PutDispEnv(&disp[db]);
    PutDrawEnv(&draw[db]);
    
    SetDispMask(1);
    
    DrawOTag(ot[db] + OTLEN - 1);
    
    db = !db;
    
    nextpri = primbuff[db];
}

void loadTexture(u_long *tim, TIM_IMAGE *tparam){
    OpenTIM(tim);
    ReadTIM(tparam);
    
    LoadImage(tparam->prect, (u_long *)tparam->paddr);
    DrawSync(0);

    if(tparam->mode & 0x8){
        LoadImage(tparam->crect, (u_long *)tparam->caddr);
        DrawSync(0);
        }
}

void pivotPoint(SVECTOR VertPos[3],short width,short height, SVECTOR pivot){

        // Not very efficient I think

        VertPos[0].vx = -pivot.vx;
        VertPos[0].vy = -pivot.vy;
        VertPos[0].vz = 1;

        VertPos[1].vx = width - pivot.vx;
        VertPos[1].vy = -pivot.vy;
        VertPos[1].vz = 1;

        VertPos[2].vx = -pivot.vx;
        VertPos[2].vy = height-pivot.vy;
        VertPos[2].vz = 1;

        VertPos[3].vx = width  - pivot.vx;
        VertPos[3].vy = height - pivot.vy;
        VertPos[3].vz = 1;
}

MATRIX identity(int num)
{
   int row, col;
   MATRIX matrix;
   
   for (row = 0; row < num; row++)
   {
      for (col = 0; col < num; col++)
      {
         if (row == col)
            matrix.m[row][col] = 4096;
         else
            matrix.m[row][col] = 0;
      }
   }
   return matrix;
}

void loadAssets(void){
    
    TIM_IMAGE bowsheetTim;
    TIM_IMAGE bowbgTim;
    TIM_IMAGE menuTim;
    TIM_IMAGE instTim;
    
    loadTexture((u_long*)_binary_TIM_bowsht_tim_start, &bowsheetTim);
    loadTexture((u_long*)_binary_TIM_bowbg_tim_start,  &bowbgTim);
    
    loadTexture((u_long*)_binary_TIM_menu_tim_start,  &menuTim);
    loadTexture((u_long*)_binary_TIM_inst_tim_start,  &instTim);
    
    bowsht_mode = bowsheetTim.mode;
    bowsht_prect = *bowsheetTim.prect;
    bowsht_crect = *bowsheetTim.crect;
    
    bowbg_mode = bowbgTim.mode;
    bowbg_prect = *bowbgTim.prect;
    bowbg_crect = *bowbgTim.crect;
    
    menu_mode = menuTim.mode;
    menu_prect = *menuTim.prect;
    menu_crect = *menuTim.crect;
    
    inst_mode = instTim.mode;
    inst_prect = *instTim.prect;
    inst_crect = *instTim.crect;
    
        
    }
    
void init(void)
{
    ResetGraph(0);
        
    InitGeom();
	SetGeomOffset(0,0);
	SetGeomScreen(1);
    
    SetDefDispEnv(&disp[0], 0, 0, SCREENXRES, SCREENYRES);
    SetDefDispEnv(&disp[1], 0, SCREENYRES, SCREENXRES, SCREENYRES);
    
    SetDefDrawEnv(&draw[0], 0, SCREENYRES, SCREENXRES, SCREENYRES);
    SetDefDrawEnv(&draw[1], 0, 0, SCREENXRES, SCREENYRES);
    
    if (VMODE)
    {
        SetVideoMode(MODE_PAL);
        disp[0].screen.y += 8;
        disp[1].screen.y += 8;
        }
        
    setRGB0(&draw[0], BgColor.r, BgColor.g, BgColor.b);
    setRGB0(&draw[1], BgColor.r, BgColor.g, BgColor.b);
    
    draw[0].isbg = 1;
    draw[1].isbg = 1;
    
    
    PadInit(0);

    // Load textures
    
    loadAssets();
    
    draw[0].tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
    draw[1].tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
    
    PutDispEnv(&disp[db]);
    PutDrawEnv(&draw[db]);
    
    FntLoad(960, 0);
    FntOpen(MARGINX, SCREENYRES - MARGINY - FONTSIZE, SCREENXRES - MARGINX * 2, FONTSIZE, 0, 280 );
    
}

// VAG playback 

// Audio initialisation & functions

void initSnd(void){

    SpuInitMalloc(MALLOC_MAX, spu_malloc_rec);                      // Maximum number of blocks, mem. management table address.
    
    commonAttributes.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR);  // Mask which attributes to set
    commonAttributes.mvol.left  = 0x3fff;                           // Master volume left
    commonAttributes.mvol.right = 0x3fff;                           // see libref47.pdf, p.1058
    
    SpuSetCommonAttr(&commonAttributes);                            // set attributes
    
    SpuSetIRQ(SPU_OFF);
}

u_long sendVAGtoRAM(DWORD VAG_data_size, BYTE *VAG_data){
    u_long size;
    
    SpuSetTransferMode(SpuTransByDMA);                              // DMA transfer; can do other processing during transfer
    
    size = SpuWrite (VAG_data + sizeof(VAGhdr), VAG_data_size);     // transfer VAG_data_size bytes from VAG_data  address to sound buffer
    
    SpuIsTransferCompleted (SPU_TRANSFER_WAIT);                     // Checks whether transfer is completed and waits for completion

    return size;
}

void setVoiceAttr(DWORD pitch, long channel, unsigned long soundAddr, short vol){
    
    voiceAttributes.mask=                                   //~ Attributes (bit string, 1 bit per attribute)
    (
      SPU_VOICE_VOLL |
	  SPU_VOICE_VOLR |
	  SPU_VOICE_PITCH |
	  SPU_VOICE_WDSA |
	  SPU_VOICE_ADSR_AMODE |
	  SPU_VOICE_ADSR_SMODE |
	  SPU_VOICE_ADSR_RMODE |
	  SPU_VOICE_ADSR_AR |
	  SPU_VOICE_ADSR_DR |
	  SPU_VOICE_ADSR_SR |
	  SPU_VOICE_ADSR_RR |
	  SPU_VOICE_ADSR_SL
    );
    
    voiceAttributes.voice        = channel;                 //~ Voice (low 24 bits are a bit string, 1 bit per voice )
    
    voiceAttributes.volume.left  = vol;                  //~ Volume 
    voiceAttributes.volume.right = vol;                  //~ Volume
    
    voiceAttributes.pitch        = pitch;                   //~ Interval (set pitch)
    voiceAttributes.addr         = soundAddr;               //~ Waveform data start address
    
    voiceAttributes.a_mode       = SPU_VOICE_LINEARIncN;    //~ Attack rate mode  = Linear Increase - see libref47.pdf p.1091
    voiceAttributes.s_mode       = SPU_VOICE_LINEARIncN;    //~ Sustain rate mode = Linear Increase
    voiceAttributes.r_mode       = SPU_VOICE_LINEARDecN;    //~ Release rate mode = Linear Decrease
    
    voiceAttributes.ar           = 0x0;                     //~ Attack rate
    voiceAttributes.dr           = 0x0;                     //~ Decay rate
    voiceAttributes.rr           = 0x0;                     //~ Release rate
    voiceAttributes.sr           = 0x0;                     //~ Sustain rate
    voiceAttributes.sl           = 0xf;                     //~ Sustain level

    SpuSetVoiceAttr(&voiceAttributes);                      // set attributes
    
}

    //~ SpuSetKey(SpuOn,SPU_0CH | SPU_2CH); // Set several channels by ORing  each channel bit ; channel 0 and 2 here.


void playSFX(u_long sfx){
    SpuSetKey(SpuOn,sfx); // Play SFX on voice 2.
}


// end VAG playback

int main(void)
{
    srand(rand());
    short r = rand();
    
    int pad;
    int t = 5;
    char ammuQty = assetsLoad.ammo;
    int msgID = -1;
    
    short dir = 1;
    short hitId = 1;
    int hitCoord[5] = {0,0,0,0,0};
    
    COORD dirV[8] = {-1, -1, 1, -1, -1, 1, -1, 1, -1, 1, -1, 1, 1, -1, -1, -1};
                
    SPRT *bgSprt;
    //~ SPRT *menuSprt;
    SPRT *instSprt;
    
    
    DR_TPAGE *tpage_bg;
    DR_TPAGE *tpage_sht;
    DR_TPAGE *tpage_menu;
    DR_TPAGE *tpage_inst;

    
    
    SPRT *uiAmmu;
    SPRT *uiParch;
    TILE *debug;
    
    MATRIX IDMATRIX = identity(3);
    
    RECT targetHitbox = {0, 0, 12, 24};
    RECT arrowHitbox = {0, 0, 3, 5};
    CRCL roundHitbox = {0, 0, 6}; 
    CRCL bubbleHitbox = {0, 0, 8}; 
    
    COORD hitPos = {0, 0};
    
    
    SPRT Ammu[assetsLoad.ammo];
    
    //Robin
    
    polygon robinPoly = {
    robinPoly.poly_ft4,
    {128, 128, 128},                  // color
    42, 48,                           // width, height
    {0,0,0},                          // RotV_L
    {32, CENTERY+5, 0},                 // TransV_L
    {4096,4096,4096},                 // ScaleV_L
    {21,24,1},                        // PivotV
    {                                 // Verts[4]
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    },
    IDMATRIX,                         // Matrix
    0,0,                              // depth, flag
    8,                                // rotSpeed
    0,                                 // z-index
    0,                                  // hit      
    0,                                   //cursor
    0,                                   //timer
    aim
    };
    
    
    // parchment
    
    polygon menuPoly = {
    menuPoly.poly_ft4,
    {128, 128, 128},                  // color
    256, 240,                           // width, height
    {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
    {CENTERX, CENTERY, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
    {5120,4096,4096},                 // ScaleV_L               
    {128,120,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
    {                                 // Verts[4]
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    },
    IDMATRIX,                         // Matrix
    0,0,                              // depth, flag
    0,                                // rotSpeed
    -1,                                 // z-index
    0,                                  // hit      
    0,                                   //cursor
    0,                                   //timer
    menu
    };
    
    // parchment
    
    polygon instPoly = {
    instPoly.poly_ft4,
    {128, 128, 128},                  // color
    256, 134,                           // width, height
    {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
    {CENTERX, CENTERY, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
    {4096,4096,4096},                 // ScaleV_L               
    {128,67,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
    {                                 // Verts[4]
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    },
    IDMATRIX,                         // Matrix
    0,0,                              // depth, flag
    0,                                // rotSpeed
    0,                                 // z-index
    0,                                  // hit      
    0,                                   //cursor
    0,                                   //timer
    inst
    };
    
    // parchment
    
    polygon parchPoly = {
    parchPoly.poly_ft4,
    {128, 128, 128},                  // color
    98, 64,                           // width, height
    {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
    {CENTERX + 5, CENTERY, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
    {7168,7168,7168},                 // ScaleV_L               
    {49,32,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
    {                                 // Verts[4]
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    },
    IDMATRIX,                         // Matrix
    0,0,                              // depth, flag
    8,                                // rotSpeed
    10,                                 // z-index
    0,                                  // hit      
    0,                                   //cursor
    0,                                   //timer
    parchment
    };
    
    // parchment
    
    polygon msgPoly = {
    msgPoly.poly_ft4,
    {128, 128, 128},                  // color
    50, 16,                           // width, height
    {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
    {CENTERX - msgPoly.PivotV_L.vx, CENTERY - msgPoly.PivotV_L.vy, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
    {4096,4096,4096},                 // ScaleV_L               
    {25,8,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
    {                                 // Verts[4]
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    },
    IDMATRIX,                         // Matrix
    0,0,                              // depth, flag
    8,                                // rotSpeed
    5,                                 // z-index
    0,                                  // hit      
    0,                                   //cursor
    0,                                   //timer
    messages
    };
    
    // arrow
    
    polygon arrowPoly[assetsLoad.nbrArrows+1];
    
    for(int i = 0; i < assetsLoad.nbrArrows; i++){
        arrowPoly[i] = (polygon){
            arrowPoly[i].poly_ft4,
            {128, 128, 128},                  // color
            28, 5,                           // width, height
            {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
            {-10, -12, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
            {4096,4096,4096},                 // ScaleV_L               
            {-4,2,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
            {                                 // Verts[4]
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0}
            },
            IDMATRIX,                         // Matrix
            0,0,                              // depth, flag
            8,                                // rotSpeed
            2,                                 // z-index
            0,                                  // hit      
            0,                                   //cursor
            0,                                   //timer
            arrow
        };
    }
    // balloon
    
    polygon balloonPoly[assetsLoad.nbrBalloons]; // = {
    
    for (int i = 0; i < assetsLoad.nbrBalloons; i++){
        balloonPoly[i] = (polygon){
            balloonPoly[i].poly_ft4,
            {128, 128, 128},                  // color
            9, 21,                           // width, height
            {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
            {START_POS + (i * 12), SCREENYRES, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
            {5120,5120,5120},                 // ScaleV_L               
            {4,10,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
            {                                 // Verts[4]
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0}
            },
            IDMATRIX,                         // Matrix
            0,0,                              // depth, flag
            8,                                // rotSpeed
            -1,                                 // z-index
            0,                                  // hit      
            0,                                   //cursor
            0,                                   //timer
            balloon
        };
        srand(rand());
        balloonPoly[i].TransV_L.vx = rand()/205 + 140;
        balloonPoly[i].TransV_L.vy = rand()/147;
        
    }
    
    
    // Level 2
    
    polygon bubblePoly[assetsLoad.nbrBubbles]; // = {
    polygon beePoly[assetsLoad.nbrBubbles]; // = {

    
    for (int i = 0; i < assetsLoad.nbrBubbles; i++){
        
        // bubble
        
        bubblePoly[i] = (polygon){
            bubblePoly[i].poly_ft4,
            {128, 128, 128},                  // color
            16, 16,                           // width, height
            {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
            {CENTERX + (i * 14), CENTERY, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
            {4096,4096,4096},                 // ScaleV_L               
            {8,8,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
            {                                 // Verts[4]
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0}
            },
            IDMATRIX,                         // Matrix
            0,0,                              // depth, flag
            8,                                // rotSpeed
            -1,                                 // z-index
            0,                                  // hit      
            0,                                   //cursor
            0,                                   //timer
            bubble
        };
        
        bubblePoly[i].TransV_L.vx = rand()/158 + 80;
        bubblePoly[i].TransV_L.vy = rand()/145;
        
        // bee
        
        beePoly[i] = (polygon){
            beePoly[i].poly_ft4,
            {128, 128, 128},                  // color
            11, 16,                           // width, height
            {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
            {CENTERX + (i * 14), CENTERY, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
            {4096,4096,4096},                 // ScaleV_L               
            {5,6,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
            {                                 // Verts[4]
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0},
                {0, 0, 0}
            },
            IDMATRIX,                         // Matrix
            0,0,                              // depth, flag
            8,                                // rotSpeed
            -1,                                 // z-index
            0,                                  // hit      
            0,                                   //cursor
            0,                                   //timer
            bee
        };
    }
    // target
    
    polygon targetPoly = {
        targetPoly.poly_ft4,
        {128, 128, 128},                  // color
        12, 26,                           // width, height
        {0,0,0},                          // RotV_L                 start pos : {0,0,-1124} -target pos : {0,0,0}
        {SCREENXRES - MARGINX, CENTERY, 0},                    // TransV_L                 start pos : {-4, 0, 0} - target pos : {2, -10, 0}
        {4096,4096,4096},                 // ScaleV_L               
        {6,13,0},                        // PivotV                   start pos : {-11,2, 1} - target pos : {4,2,1}
        {                                 // Verts[4]
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0}
        },
        IDMATRIX,                         // Matrix
        0,0,                              // depth, flag
        8,                                // rotSpeed
        -1,                                 // z-index
        0,                                  // hit      
        0,                                   //cursor
        0,                                   //timer
        target
    };
    
    setAnimSheet(0, aim         , 6, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 0, 0);    // line, array[], arraySize, gridWidth, gridHeight, Xoffset, Yoffset
    setAnimSheet(1, jump        , 6, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 0, 0);    
    setAnimSheet(2, reload      , 6, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 0, 0);  
    setAnimSheet(3, reload_arrow, 3, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 0, 0);  
    setAnimSheet(4, arrow       , 1, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 0, 0);
    setAnimSheet(4, balloon     , 8, 9, SPRITESHEETGRIDYSCALE, 0, 5);  
    
    setAnimSheet(4, bubble      , 1, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 68, 0);  
    setAnimSheet(4, bee         , 3, 11, SPRITESHEETGRIDYSCALE, 84, 0);  
    setAnimSheet(4, target      , 1, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 114, 22);  
    
    //UI
    
    setAnimSheet(0, menu         , 1, 255, 192, 0, 0);  
    setAnimSheet(0, inst         , 1, 255, 134, 0, 0);  
    setAnimSheet(3, parchment    , 1, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, SPRITESHEETGRIDXSCALE * 3, 0);  
    setAnimSheet(5, ammunitions  , 1, SPRITESHEETGRIDXSCALE, SPRITESHEETGRIDYSCALE, 0, 0);  
    setAnimSheet(5, messages     , 2, 57, SPRITESHEETGRIDYSCALE, 3, -1);  
    
    currentFrame = &aim[0];
    
    // Objects
    
    pivotPoint(robinPoly.Verts, robinPoly.width, robinPoly.height, robinPoly.PivotV_L);

    pivotPoint(targetPoly.Verts, targetPoly.width, targetPoly.height, targetPoly.PivotV_L);
    
    // UI
    
    pivotPoint(menuPoly.Verts, menuPoly.width, menuPoly.height, menuPoly.PivotV_L);
    
    pivotPoint(instPoly.Verts, instPoly.width, instPoly.height, instPoly.PivotV_L);
    
    pivotPoint(parchPoly.Verts, parchPoly.width, parchPoly.height, parchPoly.PivotV_L);
    
    pivotPoint(msgPoly.Verts, msgPoly.width, msgPoly.height, msgPoly.PivotV_L);
    
    
    for (int i = 4; i >= 0; i--){
        pivotPoint(arrowPoly[i].Verts, arrowPoly[i].width, arrowPoly[i].height, arrowPoly[i].PivotV_L);        
    }
    
    for (int i = assetsLoad.nbrBalloons-1;i >= 0; i--){
        pivotPoint(balloonPoly[i].Verts, balloonPoly[i].width, balloonPoly[i].height, balloonPoly[i].PivotV_L);
    //~ pivotPoint(balloonPoly[1].Verts, balloonPoly[1].width, balloonPoly[1].height, balloonPoly[1].PivotV_L);
    }
    
    for (int i = assetsLoad.nbrBubbles-1;i >= 0; i--){
        
        pivotPoint(bubblePoly[i].Verts, bubblePoly[i].width, bubblePoly[i].height, bubblePoly[i].PivotV_L);
        pivotPoint(beePoly[i].Verts, beePoly[i].width, beePoly[i].height, beePoly[i].PivotV_L);
    }
    
    addVector(&arrowPoly[0].TransV_L, &robinPoly.TransV_L);
    arrowPoly[0].TransV_L.vy += currentFrame->Yoffset ;

    for (int i = 0; i < assetsLoad.nbrArrows; i++){
        arrowPoly[i].otz = -1;
    }
    
    //~ addVector(&parchPoly.TransV_L, &robinPoly.TransV_L);
        
    // VAG playback
    
    const VAGhdr * VAGfileHeader = (VAGhdr *) _binary_VAG_vil_vag_start;   // get header of VAG file
    const VAGhdr * VAG1fileHeader = (VAGhdr *) _binary_VAG_pop_vag_start;   // get header of VAG file
    const VAGhdr * VAG2fileHeader = (VAGhdr *) _binary_VAG_ready_vag_start;   // get header of VAG file
    const VAGhdr * VAG3fileHeader = (VAGhdr *) _binary_VAG_shoot_vag_start;   // get header of VAG file
    const VAGhdr * VAG4fileHeader = (VAGhdr *) _binary_VAG_jmp_vag_start;   // get header of VAG file
    const VAGhdr * VAG5fileHeader = (VAGhdr *) _binary_VAG_crowd_vag_start;   // get header of VAG file
    const VAGhdr * VAG6fileHeader = (VAGhdr *) _binary_VAG_folks_vag_start;   // get header of VAG file
    const VAGhdr * VAG7fileHeader = (VAGhdr *) _binary_VAG_toc_vag_start;   // get header of VAG file
    
    // From libover47.pdf :
    //~ The sampling frequency of the original audio file can be used to determine the pitch
    //~ at which to play the VAG. pitch = (sampling frequency << 12)/44100L 
    //~ Ex: 44.1kHz=0x1000 22.05kHz=0x800 etc
    
    DWORD pitch =  (SWAP_ENDIAN32(VAGfileHeader->samplingFrequency) << 12) / 44100L;    // determinate pitch based on header "Sampling frequency" field
    DWORD pitch1 = (SWAP_ENDIAN32(VAG1fileHeader->samplingFrequency) << 12) / 44100L;    // 
    DWORD pitch2 = (SWAP_ENDIAN32(VAG2fileHeader->samplingFrequency) << 12) / 44100L;    // 
    DWORD pitch3 = (SWAP_ENDIAN32(VAG3fileHeader->samplingFrequency) << 12) / 44100L;    // 
    DWORD pitch4 = (SWAP_ENDIAN32(VAG4fileHeader->samplingFrequency) << 12) / 44100L;    // 
    DWORD pitch5 = (SWAP_ENDIAN32(VAG5fileHeader->samplingFrequency) << 12) / 44100L;    // 
    DWORD pitch6 = (SWAP_ENDIAN32(VAG6fileHeader->samplingFrequency) << 12) / 44100L;    // 
    DWORD pitch7 = (SWAP_ENDIAN32(VAG7fileHeader->samplingFrequency) << 12) / 44100L;    // 
    
    //~ DWORD pitch = 0x1000; // 44100 khz
    //~ DWORD pitch = 0x800;  // 22050 khz
    //~ DWORD pitch = 0x400;  // 11025 khz
    
    SpuInit();                                                                            // Initialize SPU. Called only once.
    
    initSnd();
    
    // Music : voice 0
    
    vag_spu_address   = SpuMalloc(SWAP_ENDIAN32(VAGfileHeader->dataSize));                // Allocate an area of dataSize bytes in the sound buffer. 
    
    spu_start_address = SpuSetTransferStartAddr(vag_spu_address);                         // Sets a starting address in the sound buffer
    
    get_start_addr    = SpuGetTransferStartAddr();                                        // SpuGetTransferStartAddr() returns current sound buffer transfer start address.
    
    transSize         = sendVAGtoRAM(SWAP_ENDIAN32(VAGfileHeader->dataSize), _binary_VAG_vil_vag_start);
    
    // set VAG to voice 0 
    
    setVoiceAttr(pitch, MUSIC, vag_spu_address, 0x0800);
   
    // Balloon pop - voice 4
    
    vag1_spu_address   = SpuMalloc((SWAP_ENDIAN32(VAG1fileHeader->dataSize)));

    spu1_start_address = SpuSetTransferStartAddr(vag1_spu_address);                       
    
    get1_start_addr    = SpuGetTransferStartAddr();                                       
    
    trans1Size         = sendVAGtoRAM(SWAP_ENDIAN32(VAG1fileHeader->dataSize), _binary_VAG_pop_vag_start);
    
    // Let's have several variations of the POP sound
    setVoiceAttr(0x1000 , POP, vag1_spu_address, 0x450);
    //~ setVoiceAttr(0x1600 , SPU_5CH, vag1_spu_address, 400);
    //~ setVoiceAttr(0x900  , SPU_6CH, vag1_spu_address, 500);
   
    // Ready arrow - voice 2
    
    vag2_spu_address   = SpuMalloc((SWAP_ENDIAN32(VAG2fileHeader->dataSize)));

    spu2_start_address = SpuSetTransferStartAddr(vag2_spu_address);                       
    
    get2_start_addr    = SpuGetTransferStartAddr();                                       

    trans2Size         = sendVAGtoRAM(SWAP_ENDIAN32(VAG2fileHeader->dataSize), _binary_VAG_ready_vag_start);
    
    setVoiceAttr(pitch2  , READY, vag2_spu_address, 0x400);
    
    // Shoot arrow - voice 3
    
    vag3_spu_address   = SpuMalloc((SWAP_ENDIAN32(VAG3fileHeader->dataSize)));

    spu3_start_address = SpuSetTransferStartAddr(vag3_spu_address);                      
    
    get3_start_addr    = SpuGetTransferStartAddr();                                      

    trans3Size         = sendVAGtoRAM(SWAP_ENDIAN32(VAG3fileHeader->dataSize), _binary_VAG_shoot_vag_start);
    
    setVoiceAttr(pitch3  , SHOOT, vag3_spu_address, 0x400);
    
    // Jump - voice 5
    
    vag4_spu_address   = SpuMalloc((SWAP_ENDIAN32(VAG4fileHeader->dataSize)));

    spu4_start_address = SpuSetTransferStartAddr(vag4_spu_address);
    
    get4_start_addr    = SpuGetTransferStartAddr();   
    
    trans4Size         = sendVAGtoRAM(SWAP_ENDIAN32(VAG4fileHeader->dataSize), _binary_VAG_jmp_vag_start);
    
    setVoiceAttr(pitch4  , JMP, vag4_spu_address, 0x300);

    // Gameover - voice 5
    
    vag5_spu_address   = SpuMalloc((SWAP_ENDIAN32(VAG5fileHeader->dataSize)));

    spu5_start_address = SpuSetTransferStartAddr(vag5_spu_address);                       
    
    get5_start_addr    = SpuGetTransferStartAddr();   
    
    trans5Size         = sendVAGtoRAM(SWAP_ENDIAN32(VAG5fileHeader->dataSize), _binary_VAG_crowd_vag_start);
    
    setVoiceAttr(pitch5  , CROWD, vag5_spu_address, 0x0700);
    
    // Gameover bad - voice 6
    
    vag6_spu_address   = SpuMalloc((SWAP_ENDIAN32(VAG6fileHeader->dataSize)));

    spu6_start_address = SpuSetTransferStartAddr(vag6_spu_address);                       
    
    get6_start_addr    = SpuGetTransferStartAddr();   
    
    trans6Size         = sendVAGtoRAM(SWAP_ENDIAN32(VAG6fileHeader->dataSize), _binary_VAG_folks_vag_start);
    
    setVoiceAttr(pitch6  , FOLKS, vag6_spu_address, 0x1000);
    
    // Toc - voice 7
    
    vag7_spu_address   = SpuMalloc((SWAP_ENDIAN32(VAG7fileHeader->dataSize)));

    spu7_start_address = SpuSetTransferStartAddr(vag7_spu_address);                       
    
    get7_start_addr    = SpuGetTransferStartAddr();   
    
    trans7Size         = sendVAGtoRAM(SWAP_ENDIAN32(VAG7fileHeader->dataSize), _binary_VAG_toc_vag_start);
    
    setVoiceAttr(0x800  , TOC, vag7_spu_address, 0x0700);

    playSFX(CROWD);
    
    // end VAG playback
    
    
    init();
    
    while (1)
    {
        ClearOTagR(ot[db], OTLEN);
        
        // menu
        
        if(!currentLvl && !gameStarted){
            menuPoly.otz = 0;
        }
        
        
        menuPoly.poly_ft4 = (POLY_FT4 *)nextpri;
        
        //menuPoly.RotV_L.vz += parchKeyframes[0].vz;
        //~ menuPoly.TransV_L.vy += currentFrame->Yoffset ;
                
        RotMatrix(&menuPoly.RotV_L, &menuPoly.Matrix);
        TransMatrix(&menuPoly.Matrix, &menuPoly.TransV_L );
        ScaleMatrix(&menuPoly.Matrix, &menuPoly.ScaleV_L);
        
        SetRotMatrix(&menuPoly.Matrix);
        SetTransMatrix(&menuPoly.Matrix);
        
        setPolyFT4(menuPoly.poly_ft4);
        menuPoly.poly_ft4->tpage = getTPage(menu_mode&0x3, 0, menu_prect.x, menu_prect.y);
        //~ //menuPoly.poly_ft4->clut = getClut(0,960);
        setRGB0(menuPoly.poly_ft4, menuPoly.color.r,menuPoly.color.g,menuPoly.color.b);   
        RotTransPers4(
                    &menuPoly.Verts[0],      &menuPoly.Verts[1],      &menuPoly.Verts[2],      &menuPoly.Verts[3],
                    (long*)&menuPoly.poly_ft4->x0, (long*)&menuPoly.poly_ft4->x1, (long*)&menuPoly.poly_ft4->x2, (long*)&menuPoly.poly_ft4->x3,
                    &menuPoly.depth,
                    &menuPoly.flag
                    );

        //~ setUV4(menuPoly.poly_ft4,  menuPoly.anim[0].u0                 , menuPoly.anim[0].v0,
                                   //~ menuPoly.anim[0].u0 + menuPoly.width, menuPoly.anim[0].v0,
                                   //~ menuPoly.anim[0].u0                 , menuPoly.anim[0].v0 + menuPoly.height,
                                   //~ menuPoly.anim[0].u0 + menuPoly.width, menuPoly.anim[0].v0 + menuPoly.height
                               //~ );
        setUV4(menuPoly.poly_ft4,  0                   , 0,
                                   menuPoly.width-1    , 0,
                                   0                   , menuPoly.height,
                                   menuPoly.width-1    , menuPoly.height
                               );
                               
        addPrim(ot[db]+menuPoly.otz, menuPoly.poly_ft4);
        
        nextpri += sizeof(POLY_FT4);

        tpage_menu = (DR_TPAGE*)nextpri;
        
        setDrawTPage(tpage_menu, 0, 1,       // Set TPage primitive
            getTPage(menu_mode&0x3, 0, 
            menu_prect.x, menu_prect.y));

        addPrim(ot[db], tpage_menu);         // Sort primitive to OT

        nextpri += sizeof(DR_TPAGE);    // Advance next primitive address
        
        //Instructions
        
        instPoly.poly_ft4 = (POLY_FT4 *)nextpri;
        
        //instPoly.RotV_L.vz += parchKeyframes[0].vz;
        //~ instPoly.TransV_L.vy += currentFrame->Yoffset ;
                
        RotMatrix(&instPoly.RotV_L, &instPoly.Matrix);
        TransMatrix(&instPoly.Matrix, &instPoly.TransV_L );
        ScaleMatrix(&instPoly.Matrix, &instPoly.ScaleV_L);
        
        SetRotMatrix(&instPoly.Matrix);
        SetTransMatrix(&instPoly.Matrix);
        
        setPolyFT4(instPoly.poly_ft4);
        instPoly.poly_ft4->tpage = getTPage(inst_mode&0x3, 0, inst_prect.x, inst_prect.y);
        //~ //instPoly.poly_ft4->clut = getClut(0,960);
        setRGB0(instPoly.poly_ft4, instPoly.color.r,instPoly.color.g,instPoly.color.b);   
        RotTransPers4(
                    &instPoly.Verts[0],      &instPoly.Verts[1],      &instPoly.Verts[2],      &instPoly.Verts[3],
                    (long*)&instPoly.poly_ft4->x0, (long*)&instPoly.poly_ft4->x1, (long*)&instPoly.poly_ft4->x2, (long*)&instPoly.poly_ft4->x3,
                    &instPoly.depth,
                    &instPoly.flag
                    );

        //~ setUV4(instPoly.poly_ft4,  instPoly.anim[0].u0                 , instPoly.anim[0].v0,
                                   //~ instPoly.anim[0].u0 + instPoly.width, instPoly.anim[0].v0,
                                   //~ instPoly.anim[0].u0                 , instPoly.anim[0].v0 + instPoly.height,
                                   //~ instPoly.anim[0].u0 + instPoly.width, instPoly.anim[0].v0 + instPoly.height
                               //~ );
        setUV4(instPoly.poly_ft4,  0                   , 0,
                                   instPoly.width-1    , 0,
                                   0                   , instPoly.height,
                                   instPoly.width-1    , instPoly.height
                               );
                               
        addPrim(ot[db]+instPoly.otz, instPoly.poly_ft4);
        
        nextpri += sizeof(POLY_FT4);

        tpage_inst = (DR_TPAGE*)nextpri;
        
        setDrawTPage(tpage_inst, 0, 1,       // Set TPage primitive
            getTPage(inst_mode&0x3, 0, 
            inst_prect.x, inst_prect.y));

        addPrim(ot[db], tpage_inst);         // Sort primitive to OT

        nextpri += sizeof(DR_TPAGE);    // Advance next primitive address
        
        
        //~ // debug shape
        
        //~ debug = (TILE *)nextpri;

        //~ setTile(debug);                  // Initialize the primitive (very important)
        //~ setXY0(debug, targetHitbox.x , targetHitbox.y);             // Position the sprite
        //~ setWH(debug, targetPoly.width, targetPoly.height);   // Set size to SCREENXRES
        //~ setRGB0(debug,                   // Set primitive color
            //~ 255, 0, 255);
        //~ addPrim(ot[db], debug);          // Sort primitive to OT

        //~ nextpri += sizeof(TILE);        // Advance next primitive address
        
        
        //~ // UI messages fg
        
        msgPoly.poly_ft4 = (POLY_FT4 *)nextpri;
        
        //msgPoly.RotV_L.vz += parchKeyframes[0].vz;
        //~ msgPoly.TransV_L.vy += currentFrame->Yoffset ;
                
        RotMatrix(&msgPoly.RotV_L, &msgPoly.Matrix);
        TransMatrix(&msgPoly.Matrix, &msgPoly.TransV_L );
        ScaleMatrix(&msgPoly.Matrix, &msgPoly.ScaleV_L);
        
        SetRotMatrix(&msgPoly.Matrix);
        SetTransMatrix(&msgPoly.Matrix);
        
        setPolyFT4(msgPoly.poly_ft4);
        msgPoly.poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
        //~ //msgPoly.poly_ft4->clut = getClut(0,960);
        setRGB0(msgPoly.poly_ft4, msgPoly.color.r,msgPoly.color.g,msgPoly.color.b);   
        RotTransPers4(
                    &msgPoly.Verts[0],      &msgPoly.Verts[1],      &msgPoly.Verts[2],      &msgPoly.Verts[3],
                    (long*)&msgPoly.poly_ft4->x0, (long*)&msgPoly.poly_ft4->x1, (long*)&msgPoly.poly_ft4->x2, (long*)&msgPoly.poly_ft4->x3,
                    &msgPoly.depth,
                    &msgPoly.flag
                    );

        setUV4(msgPoly.poly_ft4,   messages[msgID].u0                , messages[msgID].v0,
                                   messages[msgID].u0 + msgPoly.width, messages[msgID].v0,
                                   messages[msgID].u0                , messages[msgID].v0 + msgPoly.height,
                                   messages[msgID].u0 + msgPoly.width, messages[msgID].v0 + msgPoly.height
                               );
                               
        addPrim(ot[db]+msgPoly.otz, msgPoly.poly_ft4);
        
        nextpri += sizeof(POLY_FT4);
        
        // UI message bg : parchment 
        
        parchPoly.poly_ft4 = (POLY_FT4 *)nextpri;
        
        //parchPoly.RotV_L.vz += parchKeyframes[0].vz;
        //~ parchPoly.TransV_L.vy += currentFrame->Yoffset ;
                
        RotMatrix(&parchPoly.RotV_L, &parchPoly.Matrix);
        TransMatrix(&parchPoly.Matrix, &parchPoly.TransV_L );
        ScaleMatrix(&parchPoly.Matrix, &parchPoly.ScaleV_L);
        
        SetRotMatrix(&parchPoly.Matrix);
        SetTransMatrix(&parchPoly.Matrix);
        
        setPolyFT4(parchPoly.poly_ft4);
        parchPoly.poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
        //~ //parchPoly.poly_ft4->clut = getClut(0,960);
        setRGB0(parchPoly.poly_ft4, parchPoly.color.r,parchPoly.color.g,parchPoly.color.b);   
        RotTransPers4(
                    &parchPoly.Verts[0],      &parchPoly.Verts[1],      &parchPoly.Verts[2],      &parchPoly.Verts[3],
                    (long*)&parchPoly.poly_ft4->x0, (long*)&parchPoly.poly_ft4->x1, (long*)&parchPoly.poly_ft4->x2, (long*)&parchPoly.poly_ft4->x3,
                    &parchPoly.depth,
                    &parchPoly.flag
                    );

        setUV4(parchPoly.poly_ft4, parchment[0].u0                  , parchment[0].v0,
                                   parchment[0].u0 + parchPoly.width, parchment[0].v0,
                                   parchment[0].u0                  , parchment[0].v0 + parchPoly.height,
                                   parchment[0].u0 + parchPoly.width, parchment[0].v0 + parchPoly.height
                               );
                               
        addPrim(ot[db]+parchPoly.otz, parchPoly.poly_ft4);
        
        nextpri += sizeof(POLY_FT4);
        
        
        // UI ammunitions
        
        for(int i = 0; i < ammuQty; i++ ){
            
            short Xpos = SCREENXRES - 10;
            short Ypos = 5;
            
            uiAmmu = (SPRT*)nextpri;

            setSprt(uiAmmu);                      // Initialize the primitive (very important)
            setXY0(uiAmmu, Xpos - (i * 5) , Ypos);    // Position the sprite
            setWH(uiAmmu, 3, 9);          
            setClut(uiAmmu, bowsht_crect.x, bowsht_crect.y),        // Set size to SCREENXRES
            setUV0(uiAmmu, ammunitions[0].u0, ammunitions[0].v0);
            setRGB0(uiAmmu,                       // Set primitive color
                128, 128, 128);
                
            addPrim(ot[db], uiAmmu);              // Sort primitive to OT

            nextpri += sizeof(SPRT);             // Advance next primitive address
        
        }
        
        //~ nextpri = spawnArrow(&arrowPoly, ot[1], nextpri, currentFrame, bowsht_mode, bowsht_prect);
        

        
        /// arrow
        
        arrowPoly[0].poly_ft4 = (POLY_FT4 *)nextpri;
        
        //arrowPoly[0].RotV_L.vz += arrowKeyframes[0].vz;
                
        RotMatrix(&arrowPoly[0].RotV_L, &arrowPoly[0].Matrix);
        TransMatrix(&arrowPoly[0].Matrix, &arrowPoly[0].TransV_L );
        ScaleMatrix(&arrowPoly[0].Matrix, &arrowPoly[0].ScaleV_L);
        
        SetRotMatrix(&arrowPoly[0].Matrix);
        SetTransMatrix(&arrowPoly[0].Matrix);
        
        setPolyFT4(arrowPoly[0].poly_ft4);
        arrowPoly[0].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
        //~ //arrowPoly[0].poly_ft4->clut = getClut(0,960);
        setRGB0(arrowPoly[0].poly_ft4, arrowPoly[0].color.r,arrowPoly[0].color.g,arrowPoly[0].color.b);   
        RotTransPers4(
                    &arrowPoly[0].Verts[0],      &arrowPoly[0].Verts[1],      &arrowPoly[0].Verts[2],      &arrowPoly[0].Verts[3],
                    (long*)&arrowPoly[0].poly_ft4->x0, (long*)&arrowPoly[0].poly_ft4->x1, (long*)&arrowPoly[0].poly_ft4->x2, (long*)&arrowPoly[0].poly_ft4->x3,
                    &arrowPoly[0].depth,
                    &arrowPoly[0].flag
                    );

        setUV4(arrowPoly[0].poly_ft4, arrowPoly[0].anim[0].u0                  , arrowPoly[0].anim[0].v0,
                                   arrowPoly[0].anim[0].u0 + arrowPoly[0].width, arrowPoly[0].anim[0].v0,
                                   arrowPoly[0].anim[0].u0                  , arrowPoly[0].anim[0].v0 + arrowPoly[0].height,
                                   arrowPoly[0].anim[0].u0 + arrowPoly[0].width, arrowPoly[0].anim[0].v0 + arrowPoly[0].height
                               );
                               
        addPrim(ot[db]+arrowPoly[0].otz, arrowPoly[0].poly_ft4);
        
        nextpri += sizeof(POLY_FT4);
        

        

        ///// Robin
        
        robinPoly.poly_ft4 = (POLY_FT4 *)nextpri;
        
        robinPoly.TransV_L.vy += currentFrame->Yoffset ;
        
        
        RotMatrix(&robinPoly.RotV_L, &robinPoly.Matrix);
        TransMatrix(&robinPoly.Matrix, &robinPoly.TransV_L);
        ScaleMatrix(&robinPoly.Matrix, &robinPoly.ScaleV_L);
        
        SetRotMatrix(&robinPoly.Matrix);
        SetTransMatrix(&robinPoly.Matrix);
        
        setPolyFT4(robinPoly.poly_ft4);
        robinPoly.poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
        robinPoly.poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
        setClut(robinPoly.poly_ft4, bowsht_crect.x, bowsht_crect.y);
        setRGB0(robinPoly.poly_ft4, robinPoly.color.r,robinPoly.color.g,robinPoly.color.b);   
        RotTransPers4(
                    &robinPoly.Verts[0],      &robinPoly.Verts[1],      &robinPoly.Verts[2],      &robinPoly.Verts[3],
                    (long*)&robinPoly.poly_ft4->x0, (long*)&robinPoly.poly_ft4->x1, (long*)&robinPoly.poly_ft4->x2, (long*)&robinPoly.poly_ft4->x3,
                    &robinPoly.depth,
                    &robinPoly.flag
                    );

        setUV4(robinPoly.poly_ft4, currentFrame->u0,                         currentFrame->v0,
                                   currentFrame->u0 + SPRITESHEETGRIDXSCALE, currentFrame->v0,
                                   currentFrame->u0,                         currentFrame->v0 + SPRITESHEETGRIDYSCALE,
                                   currentFrame->u0 + SPRITESHEETGRIDXSCALE, currentFrame->v0 + SPRITESHEETGRIDYSCALE
                               );
                               
        addPrim(ot[db]+robinPoly.otz, robinPoly.poly_ft4);
        
        nextpri += sizeof(POLY_FT4);
        
        
        for (short m = assetsLoad.nbrBalloons-1;m >= 0; m--){
            
            balloonPoly[m].poly_ft4 = (POLY_FT4 *)nextpri;
            
            //balloonPoly[m].RotV_L.vz += arrowKeyframes[0].vz;
            //~ //balloonPoly[m].TransV_L.vy += currentFrame->Yoffset ;
                    
            RotMatrix(&balloonPoly[m].RotV_L, &balloonPoly[m].Matrix);
            TransMatrix(&balloonPoly[m].Matrix, &balloonPoly[m].TransV_L );
            ScaleMatrix(&balloonPoly[m].Matrix, &balloonPoly[m].ScaleV_L);
            
            SetRotMatrix(&balloonPoly[m].Matrix);
            SetTransMatrix(&balloonPoly[m].Matrix);
            
            setPolyFT4(balloonPoly[m].poly_ft4);
            balloonPoly[m].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
            balloonPoly[m].poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
            setClut(balloonPoly[m].poly_ft4,bowsht_crect.x, bowsht_crect.y);
            setRGB0(balloonPoly[m].poly_ft4, balloonPoly[m].color.r,balloonPoly[m].color.g,balloonPoly[m].color.b);   
            RotTransPers4(
                        &balloonPoly[m].Verts[0],      &balloonPoly[m].Verts[1],      &balloonPoly[m].Verts[2],      &balloonPoly[m].Verts[3],
                        (long*)&balloonPoly[m].poly_ft4->x0, (long*)&balloonPoly[m].poly_ft4->x1, (long*)&balloonPoly[m].poly_ft4->x2, (long*)&balloonPoly[m].poly_ft4->x3,
                        &balloonPoly[m].depth,
                        &balloonPoly[m].flag
                        );
            //~ //setUV4(balloonPoly[m].poly_ft4, 0,0, 42,0,0,48,42,48);
            setUV4(balloonPoly[m].poly_ft4,  balloonPoly[m].anim[0].u0                       , balloonPoly[m].anim[0].v0,
                                             balloonPoly[m].anim[0].u0 + balloonPoly[m].width, balloonPoly[m].anim[0].v0,
                                             balloonPoly[m].anim[0].u0                       , balloonPoly[m].anim[0].v0 + balloonPoly[m].height,
                                             balloonPoly[m].anim[0].u0 + balloonPoly[m].width, balloonPoly[m].anim[0].v0 + balloonPoly[m].height
                                   );
                                   
            addPrim(ot[db]+balloonPoly[m].otz, balloonPoly[m].poly_ft4);
            
            nextpri += sizeof(POLY_FT4);
        }
        
        for (int m = assetsLoad.nbrBubbles-1;m >= 0; m--){
            
            bubblePoly[m].poly_ft4 = (POLY_FT4 *)nextpri;
                
                //~ bubblePoly[m].TransV_L.vy += currentFrame->Yoffset ;
                
                
            RotMatrix(&bubblePoly[m].RotV_L, &bubblePoly[m].Matrix);
            TransMatrix(&bubblePoly[m].Matrix, &bubblePoly[m].TransV_L);
            ScaleMatrix(&bubblePoly[m].Matrix, &bubblePoly[m].ScaleV_L);
            
            SetRotMatrix(&bubblePoly[m].Matrix);
            SetTransMatrix(&bubblePoly[m].Matrix);
            
            setPolyFT4(bubblePoly[m].poly_ft4);
            bubblePoly[m].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
            bubblePoly[m].poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
            setClut(bubblePoly[m].poly_ft4, bowsht_crect.x, bowsht_crect.y);
            setRGB0(bubblePoly[m].poly_ft4, bubblePoly[m].color.r,bubblePoly[m].color.g,bubblePoly[m].color.b);   
            RotTransPers4(
                        &bubblePoly[m].Verts[0],      &bubblePoly[m].Verts[1],      &bubblePoly[m].Verts[2],      &bubblePoly[m].Verts[3],
                        (long*)&bubblePoly[m].poly_ft4->x0, (long*)&bubblePoly[m].poly_ft4->x1, (long*)&bubblePoly[m].poly_ft4->x2, (long*)&bubblePoly[m].poly_ft4->x3,
                        &bubblePoly[m].depth,
                        &bubblePoly[m].flag
                        );

            setUV4(bubblePoly[m].poly_ft4,bubblePoly[m].anim[0].u0 ,                            bubblePoly[m].anim[0].v0,
                                          bubblePoly[m].anim[0].u0 + bubblePoly[m].width     , bubblePoly[m].anim[0].v0,
                                          bubblePoly[m].anim[0].u0,                            bubblePoly[m].anim[0].v0 + bubblePoly[m].height     ,
                                          bubblePoly[m].anim[0].u0 + bubblePoly[m].width     , bubblePoly[m].anim[0].v0 + bubblePoly[m].height
                                   );
                                   
            addPrim(ot[db]+bubblePoly[m].otz, bubblePoly[m].poly_ft4);
            
            nextpri += sizeof(POLY_FT4);
        
            ///// Bee
            
            beePoly[m].poly_ft4 = (POLY_FT4 *)nextpri;
            
            //~ //beePoly.TransV_L.vy += currentFrame->Yoffset ;
            
            
            RotMatrix(&beePoly[m].RotV_L, &beePoly[m].Matrix);
            TransMatrix(&beePoly[m].Matrix, &beePoly[m].TransV_L);
            ScaleMatrix(&beePoly[m].Matrix, &beePoly[m].ScaleV_L);
            
            SetRotMatrix(&beePoly[m].Matrix);
            SetTransMatrix(&beePoly[m].Matrix);
            
            setPolyFT4(beePoly[m].poly_ft4);
            beePoly[m].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
            beePoly[m].poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
            setClut(beePoly[m].poly_ft4, bowsht_crect.x, bowsht_crect.y);
            setRGB0(beePoly[m].poly_ft4, beePoly[m].color.r,beePoly[m].color.g,beePoly[m].color.b);   
            RotTransPers4(
                        &beePoly[m].Verts[0],      &beePoly[m].Verts[1],      &beePoly[m].Verts[2],      &beePoly[m].Verts[3],
                        (long*)&beePoly[m].poly_ft4->x0, (long*)&beePoly[m].poly_ft4->x1, (long*)&beePoly[m].poly_ft4->x2, (long*)&beePoly[m].poly_ft4->x3,
                        &beePoly[m].depth,
                        &beePoly[m].flag
                        );

            setUV4(beePoly[m].poly_ft4,    beePoly[m].anim[0].u0,                             beePoly[m].anim[0].v0,
                                           beePoly[m].anim[0].u0 + beePoly[m].width     , beePoly[m].anim[0].v0,
                                           beePoly[m].anim[0].u0,                             beePoly[m].anim[0].v0 + beePoly[m].height     ,
                                           beePoly[m].anim[0].u0 + beePoly[m].width     , beePoly[m].anim[0].v0 + beePoly[m].height
                                   );
                                   
            addPrim(ot[db]+beePoly[m].otz, beePoly[m].poly_ft4);
            
            nextpri += sizeof(POLY_FT4);
            
        }

        for (int i = 1; i < assetsLoad.nbrArrows; i++){                /// arrow
                
                arrowPoly[i].TransV_L.vx = targetPoly.TransV_L.vx - 20;
                arrowPoly[i].TransV_L.vy = targetPoly.TransV_L.vy - hitCoord[i];
                arrowPoly[i].ScaleV_L.vx = 2500;
                
                arrowPoly[i].poly_ft4 = (POLY_FT4 *)nextpri;
                
                //arrowPoly[i].RotV_L.vz += arrowKeyframes[0].vz;
                        
                RotMatrix(&arrowPoly[i].RotV_L, &arrowPoly[i].Matrix);
                TransMatrix(&arrowPoly[i].Matrix, &arrowPoly[i].TransV_L );
                ScaleMatrix(&arrowPoly[i].Matrix, &arrowPoly[i].ScaleV_L);
                
                SetRotMatrix(&arrowPoly[i].Matrix);
                SetTransMatrix(&arrowPoly[i].Matrix);
                
                setPolyFT4(arrowPoly[i].poly_ft4);
                arrowPoly[i].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
                //~ //arrowPoly[i].poly_ft4->clut = getClut(0,960);
                setRGB0(arrowPoly[i].poly_ft4, arrowPoly[i].color.r,arrowPoly[i].color.g,arrowPoly[i].color.b);   
                RotTransPers4(
                            &arrowPoly[i].Verts[0],      &arrowPoly[i].Verts[1],      &arrowPoly[i].Verts[2],      &arrowPoly[i].Verts[3],
                            (long*)&arrowPoly[i].poly_ft4->x0, (long*)&arrowPoly[i].poly_ft4->x1, (long*)&arrowPoly[i].poly_ft4->x2, (long*)&arrowPoly[i].poly_ft4->x3,
                            &arrowPoly[i].depth,
                            &arrowPoly[i].flag
                            );

                setUV4(arrowPoly[i].poly_ft4, arrowPoly[i].anim[0].u0                  , arrowPoly[i].anim[0].v0,
                                           arrowPoly[i].anim[0].u0 + arrowPoly[i].width - 5, arrowPoly[i].anim[0].v0,
                                           arrowPoly[i].anim[0].u0                  , arrowPoly[i].anim[0].v0 + arrowPoly[i].height,
                                           arrowPoly[i].anim[0].u0 + arrowPoly[i].width - 5, arrowPoly[i].anim[0].v0 + arrowPoly[i].height
                                       );
                                       
                addPrim(ot[db]+arrowPoly[i].otz, arrowPoly[i].poly_ft4);
                
                nextpri += sizeof(POLY_FT4);
                
                
            }

            ///// Target
        
            targetPoly.poly_ft4 = (POLY_FT4 *)nextpri;
            
            //~ targetPoly.TransV_L.vy += currentFrame->Yoffset ;
            
            
            RotMatrix(&targetPoly.RotV_L, &targetPoly.Matrix);
            TransMatrix(&targetPoly.Matrix, &targetPoly.TransV_L);
            ScaleMatrix(&targetPoly.Matrix, &targetPoly.ScaleV_L);
            
            SetRotMatrix(&targetPoly.Matrix);
            SetTransMatrix(&targetPoly.Matrix);
            
            setPolyFT4(targetPoly.poly_ft4);
            targetPoly.poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
            targetPoly.poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
            setClut(targetPoly.poly_ft4, bowsht_crect.x, bowsht_crect.y);
            setRGB0(targetPoly.poly_ft4, targetPoly.color.r,targetPoly.color.g,targetPoly.color.b);   
            RotTransPers4(
                        &targetPoly.Verts[0],      &targetPoly.Verts[1],      &targetPoly.Verts[2],      &targetPoly.Verts[3],
                        (long*)&targetPoly.poly_ft4->x0, (long*)&targetPoly.poly_ft4->x1, (long*)&targetPoly.poly_ft4->x2, (long*)&targetPoly.poly_ft4->x3,
                        &targetPoly.depth,
                        &targetPoly.flag
                        );

            setUV4(targetPoly.poly_ft4,target[0].u0,                         target[0].v0,
                                       target[0].u0 + targetPoly.width     , target[0].v0,
                                       target[0].u0,                         target[0].v0 + targetPoly.height     ,
                                       target[0].u0 + targetPoly.width     , target[0].v0 + targetPoly.height
                                   );
                                   
            addPrim(ot[db]+targetPoly.otz, targetPoly.poly_ft4);
            
            nextpri += sizeof(POLY_FT4);

        
        if (currentLvl == 1){
        
            nbrBalloonsHit = 0;
            
            for (short m = assetsLoad.nbrBalloons-1;m >= 0; m--){
                
                balloonPoly[m].otz = 0;
                
              // Balloon animation
              
                if(balloonPoly[m].timer == 0){
                    balloonPoly[m].anim = &balloon[balloonPoly[m].cursor];
                    balloonPoly[m].cursor += 1;
                    if(balloonPoly[m].cursor == 4){
                        balloonPoly[m].cursor = 0;
                    }
                    if(balloonPoly[m].cursor == 6 && balloonPoly[m].hit == 0){
                        if(SpuGetKeyStatus(POP) == 0 || SpuGetKeyStatus(POP) == 3 ){
                            playSFX(POP);
                        }
                    }
                    if(balloonPoly[m].cursor == 8){
                        balloonPoly[m].cursor = 7;
                        balloonPoly[m].hit = 1;
                    }
                    balloonPoly[m].timer = 5;
                }
                balloonPoly[m].timer -= 1;
                
                // Balloon roundHitbox's position 
            
                roundHitbox.x = balloonPoly[m].TransV_L.vx - (roundHitbox.r / 2);
                roundHitbox.y = balloonPoly[m].TransV_L.vy - roundHitbox.r;
                
                // If distance arrowHitbox < roundHitbox.r , hit.
                // d = sqrt((arrowHitbox.x - roundHitbox.x)² + (arrowHitbox.y - roundHitbox.y)²) // Pythagore
                // We want to avoid sqrt :
                // d² = (arrowHitbox.x - roundHitbox.x)² + (arrowHitbox.y - roundHitbox.y)²;
                // We'll use d squared for comparison with the circle's radius
                
                int d2, r2;
                d2 = ( (arrowHitbox.x - roundHitbox.x) * (arrowHitbox.x - roundHitbox.x) ) + ( (arrowHitbox.y - roundHitbox.y) * (arrowHitbox.y - roundHitbox.y) );
                r2 = ( roundHitbox.r * roundHitbox.r );
                if(d2 < r2 ){
                     balloonPoly[m].cursor = 5;
                     //~ if(arrowAnim.timer == 0){
                    //~ balloonPoly[m].otz = -1;
                     //~ }
                    }
                if(balloonPoly[m].hit == 1){
                    balloonPoly[m].otz = -1;
                    nbrBalloonsHit += 1;
                    
                                
                }
                
                if(gameStarted && !gameReset){
            
            
                    if(nbrBalloonsHit == assetsLoad.nbrBalloons && ammuQty >= 0){
                        gameStarted = 0;
                        msgID = 1;
                        currentLvl = 2;
                        }
                    if(nbrBalloonsHit >= 0 && ammuQty == 0 && !arrowReleased ){
                        gameStarted = 0;
                        msgID = 0;
                        }
                        
                    //~ if (hit){
                        //~ nbrBalloonsHit -= 1;
                        //~ hit = 0;
                        //~ }
                }
                
                // Spawn balloons
                
                //~ balloonPoly[m].poly_ft4 = (POLY_FT4 *)nextpri;
                
                //~ //balloonPoly[m].RotV_L.vz += arrowKeyframes[0].vz;
                //balloonPoly[m].TransV_L.vy += currentFrame->Yoffset ;
                        
                //~ RotMatrix(&balloonPoly[m].RotV_L, &balloonPoly[m].Matrix);
                //~ TransMatrix(&balloonPoly[m].Matrix, &balloonPoly[m].TransV_L );
                //~ ScaleMatrix(&balloonPoly[m].Matrix, &balloonPoly[m].ScaleV_L);
                
                //~ SetRotMatrix(&balloonPoly[m].Matrix);
                //~ SetTransMatrix(&balloonPoly[m].Matrix);
                
                //~ setPolyFT4(balloonPoly[m].poly_ft4);
                //~ balloonPoly[m].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
                //~ balloonPoly[m].poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
                //~ setClut(balloonPoly[m].poly_ft4,bowsht_crect.x, bowsht_crect.y);
                //~ setRGB0(balloonPoly[m].poly_ft4, balloonPoly[m].color.r,balloonPoly[m].color.g,balloonPoly[m].color.b);   
                //~ RotTransPers4(
                            //~ &balloonPoly[m].Verts[0],      &balloonPoly[m].Verts[1],      &balloonPoly[m].Verts[2],      &balloonPoly[m].Verts[3],
                            //~ (long*)&balloonPoly[m].poly_ft4->x0, (long*)&balloonPoly[m].poly_ft4->x1, (long*)&balloonPoly[m].poly_ft4->x2, (long*)&balloonPoly[m].poly_ft4->x3,
                            //~ &balloonPoly[m].depth,
                            //~ &balloonPoly[m].flag
                            //~ );
                //setUV4(balloonPoly[m].poly_ft4, 0,0, 42,0,0,48,42,48);
                //~ setUV4(balloonPoly[m].poly_ft4,  balloonPoly[m].anim[0].u0                       , balloonPoly[m].anim[0].v0,
                                                 //~ balloonPoly[m].anim[0].u0 + balloonPoly[m].width, balloonPoly[m].anim[0].v0,
                                                 //~ balloonPoly[m].anim[0].u0                       , balloonPoly[m].anim[0].v0 + balloonPoly[m].height,
                                                 //~ balloonPoly[m].anim[0].u0 + balloonPoly[m].width, balloonPoly[m].anim[0].v0 + balloonPoly[m].height
                                       //~ );
                                       
                //~ addPrim(ot[db]+balloonPoly[m].otz, balloonPoly[m].poly_ft4);
                
                //~ nextpri += sizeof(POLY_FT4);
                
            }
            
        }
        
        

               // Loop on balloons 
        
        if (currentLvl == 2){
            
            
            short playSnd = 0;
            
            for (int m = assetsLoad.nbrBubbles-1;m >= 0; m--){
                
                bubblePoly[m].otz = 0;
                beePoly[m].otz    = 0;
            
                if(bubblePoly[m].TransV_L.vx >= SCREENXRES - bubblePoly[m].PivotV_L.vx || bubblePoly[m].TransV_L.vx - bubblePoly[m].PivotV_L.vx  <= 100){
                    dirV[m].x *= -1;
                }
                if(bubblePoly[m].TransV_L.vy >= SCREENYRES - bubblePoly[m].PivotV_L.vy || bubblePoly[m].TransV_L.vy - bubblePoly[m].PivotV_L.vy  <= 0){
                    dirV[m].y *= -1;
                }
                
                
                bubblePoly[m].timer ++;
                
                if (!bubblePoly[m].hit){
                    bubblePoly[m].TransV_L.vx += dirV[m].x; 
                    bubblePoly[m].TransV_L.vy += dirV[m].y; 
                }
                // Butterfly animation
              
                if(beePoly[m].timer == 0){
                    beePoly[m].anim = &bee[beePoly[m].cursor];
                    beePoly[m].cursor += 1;
                    if(beePoly[m].cursor == 2){
                        beePoly[m].cursor = 0;
                    }
                    beePoly[m].timer = 5;
                }
                beePoly[m].timer -= 1;

                                
                // Bubbles roundHitbox's position 

                bubbleHitbox.x = bubblePoly[m].TransV_L.vx - (bubbleHitbox.r);
                bubbleHitbox.y = bubblePoly[m].TransV_L.vy - bubbleHitbox.r + 2;
                
                // If distance arrowHitbox < roundHitbox.r , hit.
                // d = sqrt((arrowHitbox.x - roundHitbox.x)² + (arrowHitbox.y - roundHitbox.y)²) // Pythagore
                // We want to avoid sqrt :
                // d² = (arrowHitbox.x - roundHitbox.x)² + (arrowHitbox.y - roundHitbox.y)²;
                // We'll use d squared for comparison with the circle's radius
                
                int d2, r2;
                d2 = ( (arrowHitbox.x - bubbleHitbox.x) * (arrowHitbox.x - bubbleHitbox.x) ) + ( (arrowHitbox.y - bubbleHitbox.y) * (arrowHitbox.y - bubbleHitbox.y) );
                r2 = ( bubbleHitbox.r * bubbleHitbox.r );
                if(d2 < r2 ){
                     //~ balloonPoly[m].cursor = 5;
                     //~ if(arrowAnim.timer == 0){
                    bubblePoly[m].otz = -1;
                    bubblePoly[m].hit = 1;
                     //~ }
                    playSnd = 1;
                    }
                if(bubblePoly[m].hit && playSnd){
                    if (SpuGetKeyStatus(POP) == 0 || SpuGetKeyStatus(POP) == 3 ){
                        playSFX(POP);
                        playSnd = 0;
                    }
                }
                if(bubblePoly[m].hit == 1 && beePoly[m].hit == 0){
                    bubblePoly[m].otz = -1;
                    beePoly[m].TransV_L.vx += 1;
                    beePoly[m].TransV_L.vy -= 1;
                    bubblePoly[m].TransV_L.vx = -20;
                    bubblePoly[m].TransV_L.vy = -20;

                    if (beePoly[m].TransV_L.vx > SCREENXRES || beePoly[m].TransV_L.vy < 0){
                        beePoly[m].otz = -1;
                        bubblePoly[m].otz = -1;
                        beePoly[m].hit = 1;
                        nbrBubblesHit += 1;
                        beePoly[m].TransV_L.vx = -20;
                        beePoly[m].TransV_L.vy = -20;
                    }
                }
                
                if (bubblePoly[m].hit == 0){
                    beePoly[m].TransV_L.vx = bubblePoly[m].TransV_L.vx;
                    beePoly[m].TransV_L.vy = bubblePoly[m].TransV_L.vy;

                }
                

                if(gameStarted && !gameReset){
            
            
                    if(nbrBubblesHit == assetsLoad.nbrBubbles && ammuQty >= 0){
                        gameStarted = 0;
                        msgID = 1;
                        currentLvl = 3;
                    }
                    if(nbrBubblesHit >= 0 && ammuQty == 0 && !arrowReleased && !bubblePoly[m].hit){
                        gameStarted = 0;
                        msgID = 0;
                        currentLvl = 1;
                    }
                        
                    //~ if (hit){
                        //~ nbrBubblesHit -= 1;
                        //~ hit = 0;
                        //~ }
                }
                    
                //~ FntPrint("%d, ", bubblePoly[m].hit);
                // Drawings
            
                //~ bubblePoly[m].poly_ft4 = (POLY_FT4 *)nextpri;
                
                //bubblePoly[m].TransV_L.vy += currentFrame->Yoffset ;
                
                
                //~ RotMatrix(&bubblePoly[m].RotV_L, &bubblePoly[m].Matrix);
                //~ TransMatrix(&bubblePoly[m].Matrix, &bubblePoly[m].TransV_L);
                //~ ScaleMatrix(&bubblePoly[m].Matrix, &bubblePoly[m].ScaleV_L);
                
                //~ SetRotMatrix(&bubblePoly[m].Matrix);
                //~ SetTransMatrix(&bubblePoly[m].Matrix);
                
                //~ setPolyFT4(bubblePoly[m].poly_ft4);
                //~ bubblePoly[m].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
                //~ bubblePoly[m].poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
                //~ setClut(bubblePoly[m].poly_ft4, bowsht_crect.x, bowsht_crect.y);
                //~ setRGB0(bubblePoly[m].poly_ft4, bubblePoly[m].color.r,bubblePoly[m].color.g,bubblePoly[m].color.b);   
                //~ RotTransPers4(
                            //~ &bubblePoly[m].Verts[0],      &bubblePoly[m].Verts[1],      &bubblePoly[m].Verts[2],      &bubblePoly[m].Verts[3],
                            //~ (long*)&bubblePoly[m].poly_ft4->x0, (long*)&bubblePoly[m].poly_ft4->x1, (long*)&bubblePoly[m].poly_ft4->x2, (long*)&bubblePoly[m].poly_ft4->x3,
                            //~ &bubblePoly[m].depth,
                            //~ &bubblePoly[m].flag
                            //~ );

                //~ setUV4(bubblePoly[m].poly_ft4,bubblePoly[m].anim[0].u0 ,                            bubblePoly[m].anim[0].v0,
                                              //~ bubblePoly[m].anim[0].u0 + bubblePoly[m].width     , bubblePoly[m].anim[0].v0,
                                              //~ bubblePoly[m].anim[0].u0,                            bubblePoly[m].anim[0].v0 + bubblePoly[m].height     ,
                                              //~ bubblePoly[m].anim[0].u0 + bubblePoly[m].width     , bubblePoly[m].anim[0].v0 + bubblePoly[m].height
                                       //~ );
                                       
                //~ addPrim(ot[db]+bubblePoly[m].otz, bubblePoly[m].poly_ft4);
                
                //~ nextpri += sizeof(POLY_FT4);
            
                //~ ///// Bee
                
                //~ beePoly[m].poly_ft4 = (POLY_FT4 *)nextpri;
                
                //beePoly.TransV_L.vy += currentFrame->Yoffset ;
                
                
                //~ RotMatrix(&beePoly[m].RotV_L, &beePoly[m].Matrix);
                //~ TransMatrix(&beePoly[m].Matrix, &beePoly[m].TransV_L);
                //~ ScaleMatrix(&beePoly[m].Matrix, &beePoly[m].ScaleV_L);
                
                //~ SetRotMatrix(&beePoly[m].Matrix);
                //~ SetTransMatrix(&beePoly[m].Matrix);
                
                //~ setPolyFT4(beePoly[m].poly_ft4);
                //~ beePoly[m].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
                //~ beePoly[m].poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
                //~ setClut(beePoly[m].poly_ft4, bowsht_crect.x, bowsht_crect.y);
                //~ setRGB0(beePoly[m].poly_ft4, beePoly[m].color.r,beePoly[m].color.g,beePoly[m].color.b);   
                //~ RotTransPers4(
                            //~ &beePoly[m].Verts[0],      &beePoly[m].Verts[1],      &beePoly[m].Verts[2],      &beePoly[m].Verts[3],
                            //~ (long*)&beePoly[m].poly_ft4->x0, (long*)&beePoly[m].poly_ft4->x1, (long*)&beePoly[m].poly_ft4->x2, (long*)&beePoly[m].poly_ft4->x3,
                            //~ &beePoly[m].depth,
                            //~ &beePoly[m].flag
                            //~ );

                //~ setUV4(beePoly[m].poly_ft4,    beePoly[m].anim[0].u0,                             beePoly[m].anim[0].v0,
                                               //~ beePoly[m].anim[0].u0 + beePoly[m].width     , beePoly[m].anim[0].v0,
                                               //~ beePoly[m].anim[0].u0,                             beePoly[m].anim[0].v0 + beePoly[m].height     ,
                                               //~ beePoly[m].anim[0].u0 + beePoly[m].width     , beePoly[m].anim[0].v0 + beePoly[m].height
                                       //~ );
                                       
                //~ addPrim(ot[db]+beePoly[m].otz, beePoly[m].poly_ft4);
                
                //~ nextpri += sizeof(POLY_FT4);
            }
            
        }
        
        if (currentLvl == 3){
            
                targetPoly.otz = 0;
            
                //~ static short dir = 1;
                //~ static short hitId = 1;
                //~ static short hitCoord[5] = {0,0,0,0,0};
               
                if(targetPoly.TransV_L.vy >= SCREENYRES - targetPoly.PivotV_L.vy 
                || targetPoly.TransV_L.vy - targetPoly.PivotV_L.vy  <= 0){ 
                    dir *= -1;
                }
                targetPoly.TransV_L.vy += dir;

                targetHitbox.x = targetPoly.TransV_L.vx - targetPoly.width/2;
                targetHitbox.y = targetPoly.TransV_L.vy - targetPoly.height/2 + 3; // error correction ?
            
                // square hitboxes 
                
                if(    arrowHitbox.x + arrowHitbox.w < targetPoly.TransV_L.vx
                    || arrowHitbox.x > targetHitbox.x + targetPoly.width
                    || arrowHitbox.y + arrowHitbox.h < targetHitbox.y
                    || arrowHitbox.y > targetHitbox.y + targetPoly.height
                  ){
                    hit = 0;
                    //~ targetPoly.hit = 0;
                    }
                else {
                    hit = 1;
                    
                    if(SpuGetKeyStatus(SHOOT) == 3){
                        SpuSetKey(SPU_OFF, SHOOT);
                    }
                    if (hitId < assetsLoad.nbrArrows){
                        hitId += 1;
                    }
                    
                    arrowPoly[0].TransV_L.vx = robinPoly.TransV_L.vx - 10 ;
                    arrowPoly[0].otz = 2;
                    arrowPoly[hitId].otz = 0;
                    arrowReleased = 0;
                    hitCoord[hitId] = targetPoly.TransV_L.vy - arrowHitbox.y;
                    playSFX(TOC);

                }
                
                if(gameStarted && !gameReset){
                
                    if ( hitCoord[hitId] >= -3 && hitCoord[hitId] <= 3 && hit == 1){
                        gameStarted = 0;
                        msgID = 1;
                        targetPoly.otz = -1;
                        //~ assetsLoad = &first;
                        currentLvl = 1;

                        for (int i = 1; i < 5; i++){
                             arrowPoly[i].otz = -1;
                        }
                    }
                

                    if( ammuQty == 0 && !arrowReleased){
                        gameStarted = 0;
                        msgID = 0;
                        currentLvl = 1;
                    }
                        
                    //~ if (hit){
                        //~ nbrBubblesHit -= 1;
                        //~ hit = 0;
                        //~ }
                }
                
                
                
                //~ FntPrint("%d\n",hit);
                //~ FntPrint("%d %d\n", hitCoord[hitId], hitId);
                
                //~ hitPos.x = targetPoly.TransV_L.vx - 25;
                //~ hitPos.y = targetPoly.TransV_L.vy - hity;
        
            //~ for (int i = 1; i < assetsLoad.nbrArrows; i++){                /// arrow
                
                //~ arrowPoly[i].TransV_L.vx = targetPoly.TransV_L.vx - 20;
                //~ arrowPoly[i].TransV_L.vy = targetPoly.TransV_L.vy - hitCoord[i];
                //~ arrowPoly[i].ScaleV_L.vx = 2500;
                
                //~ arrowPoly[i].poly_ft4 = (POLY_FT4 *)nextpri;
                
                //~ //arrowPoly[i].RotV_L.vz += arrowKeyframes[0].vz;
                        
                //~ RotMatrix(&arrowPoly[i].RotV_L, &arrowPoly[i].Matrix);
                //~ TransMatrix(&arrowPoly[i].Matrix, &arrowPoly[i].TransV_L );
                //~ ScaleMatrix(&arrowPoly[i].Matrix, &arrowPoly[i].ScaleV_L);
                
                //~ SetRotMatrix(&arrowPoly[i].Matrix);
                //~ SetTransMatrix(&arrowPoly[i].Matrix);
                
                //~ setPolyFT4(arrowPoly[i].poly_ft4);
                //~ arrowPoly[i].poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
                //arrowPoly[i].poly_ft4->clut = getClut(0,960);
                //~ setRGB0(arrowPoly[i].poly_ft4, arrowPoly[i].color.r,arrowPoly[i].color.g,arrowPoly[i].color.b);   
                //~ RotTransPers4(
                            //~ &arrowPoly[i].Verts[0],      &arrowPoly[i].Verts[1],      &arrowPoly[i].Verts[2],      &arrowPoly[i].Verts[3],
                            //~ (long*)&arrowPoly[i].poly_ft4->x0, (long*)&arrowPoly[i].poly_ft4->x1, (long*)&arrowPoly[i].poly_ft4->x2, (long*)&arrowPoly[i].poly_ft4->x3,
                            //~ &arrowPoly[i].depth,
                            //~ &arrowPoly[i].flag
                            //~ );

                //~ setUV4(arrowPoly[i].poly_ft4, arrowPoly[i].anim[0].u0                  , arrowPoly[i].anim[0].v0,
                                           //~ arrowPoly[i].anim[0].u0 + arrowPoly[i].width - 5, arrowPoly[i].anim[0].v0,
                                           //~ arrowPoly[i].anim[0].u0                  , arrowPoly[i].anim[0].v0 + arrowPoly[i].height,
                                           //~ arrowPoly[i].anim[0].u0 + arrowPoly[i].width - 5, arrowPoly[i].anim[0].v0 + arrowPoly[i].height
                                       //~ );
                                       
                //~ addPrim(ot[db]+arrowPoly[i].otz, arrowPoly[i].poly_ft4);
                
                //~ nextpri += sizeof(POLY_FT4);
                
                
            //~ }

            //~ ///// Target
        
            //~ targetPoly.poly_ft4 = (POLY_FT4 *)nextpri;
            
            //targetPoly.TransV_L.vy += currentFrame->Yoffset ;
            
            
            //~ RotMatrix(&targetPoly.RotV_L, &targetPoly.Matrix);
            //~ TransMatrix(&targetPoly.Matrix, &targetPoly.TransV_L);
            //~ ScaleMatrix(&targetPoly.Matrix, &targetPoly.ScaleV_L);
            
            //~ SetRotMatrix(&targetPoly.Matrix);
            //~ SetTransMatrix(&targetPoly.Matrix);
            
            //~ setPolyFT4(targetPoly.poly_ft4);
            //~ targetPoly.poly_ft4->tpage = getTPage(bowsht_mode&0x3, 0, bowsht_prect.x, bowsht_prect.y);
            //~ targetPoly.poly_ft4->clut = getClut(bowsht_crect.x, bowsht_crect.y);
            //~ setClut(targetPoly.poly_ft4, bowsht_crect.x, bowsht_crect.y);
            //~ setRGB0(targetPoly.poly_ft4, targetPoly.color.r,targetPoly.color.g,targetPoly.color.b);   
            //~ RotTransPers4(
                        //~ &targetPoly.Verts[0],      &targetPoly.Verts[1],      &targetPoly.Verts[2],      &targetPoly.Verts[3],
                        //~ (long*)&targetPoly.poly_ft4->x0, (long*)&targetPoly.poly_ft4->x1, (long*)&targetPoly.poly_ft4->x2, (long*)&targetPoly.poly_ft4->x3,
                        //~ &targetPoly.depth,
                        //~ &targetPoly.flag
                        //~ );

            //~ setUV4(targetPoly.poly_ft4,target[0].u0,                         target[0].v0,
                                       //~ target[0].u0 + targetPoly.width     , target[0].v0,
                                       //~ target[0].u0,                         target[0].v0 + targetPoly.height     ,
                                       //~ target[0].u0 + targetPoly.width     , target[0].v0 + targetPoly.height
                                   //~ );
                                   
            //~ addPrim(ot[db]+targetPoly.otz, targetPoly.poly_ft4);
            
            //~ nextpri += sizeof(POLY_FT4);
            
            
            
        }

        tpage_sht = (DR_TPAGE*)nextpri;
        
        setDrawTPage(tpage_sht, 0, 1,       // Set TPage primitive
            getTPage(bowsht_mode&0x3, 0, 
            bowsht_prect.x, bowsht_prect.y));

        addPrim(ot[db], tpage_sht);         // Sort primitive to OT

        nextpri += sizeof(DR_TPAGE);    // Advance next primitive address
        

        // background
        
        bgSprt = (SPRT*)nextpri;

        setSprt(bgSprt);                  // Initialize the primitive (very important)
        setXY0(bgSprt, 0, 0);             // Position the sprite
        setWH(bgSprt, 256, SCREENYRES);   // Set size to SCREENXRES
        setClut(bgSprt, bowbg_crect.x, bowbg_crect.y);
        setUV0(bgSprt, 0, 0);
        setRGB0(bgSprt,                   // Set primitive color
            128, 128, 128);
            
        addPrim(ot[db], bgSprt);          // Sort primitive to OT

        nextpri += sizeof(SPRT);        // Advance next primitive address

        tpage_bg = (DR_TPAGE*)nextpri;
        
        setDrawTPage(tpage_bg, 0, 1,       // Set TPage primitive
            getTPage(bowbg_mode&0x3, 0, 
            bowbg_prect.x, bowbg_prect.y));

        addPrim(ot[db], tpage_bg);         // Sort primitive to OT

        nextpri += sizeof(DR_TPAGE);    // Advance next primitive address
        

        // update arrow hitbox pos
        
        arrowHitbox.x = arrowPoly[0].TransV_L.vx + 30;
        arrowHitbox.y = arrowPoly[0].TransV_L.vy - 2;
        
        
        
        if(buttonTimer){                // button timer limits input rate
                buttonTimer --;
            }
        // generic input event handler
                
        pad = PadRead(0);
                
        // O button
        
        if (pad & PADRright){   // button is pressed
           if (!(pad & PADLdown) && !(pad & PADLup)){
                btnRight = 1;
            }
        }
        if (!(pad & PADRright)){ // button is not pressed
           btnRight = 0;
           
        }
        
        //~ if (pad & PADRdown){
           //~ btnLeft = 1;
        //~ }
        //~ if (!(pad & PADRdown)){
           //~ btnLeft = 0;
        //~ }
        
        // D-pad up and down
        
        if (pad & PADLup){
            if (!(pad & PADLdown) && !(pad & PADRright)){
                btnUp = 1;
            }
        }
        
        if (!(pad & PADLup)){
           btnUp = 0;
        }
        if (pad & PADLdown){
            if (!(pad & PADRright) && !(pad & PADLup)){
                btnDown = 1;
            }
        }
        if (!(pad & PADLdown)){
           btnDown = 0;
        }
        
        ////////
        
        if(!gameStarted){
            if(btnRight){
                buttonRActive = 1;
                buttonTimer = 30;
            }
            if(!btnRight && buttonRActive){
                //~ if(currentLvl == 0) {
                    gameReset = 1;
                //~ }

                buttonRActive = 0;
                buttonTimer = 30;
                
                }
        }
        
        if(gameStarted && ammuQty > 0){

        //send animation frames to queue
        
        
            if(btnRight && !buttonRActive){ 
                if(SpuGetKeyStatus(READY) == 0 && animPlayerQueueSize == 0){
                    playSFX(READY);
                }
                if (animPlayerQueueSize == 0){
                    if(!buttonTimer){
                        char i;
                        for(i = 0; i < sizeof(aim)/sizeof(frame); i++){
                            *nextFrame = aim[i];
                            nextFrame ++;
                        }
                        buttonRActive = 1;
                        buttonTimer = 30;
                    }
                }
                if(intro){
                    intro = 0;
                    instPoly.otz = -1;
                    }
            }
            
            //~ if(btnRight && buttonRActive){
                //~ if (currentFrame->index == 5){
                    //~ arrowPoly.otz = 0;
                  //~ }
                //~ }
            
            if(!btnRight && buttonRActive){
                    //~ playSFX(SHOOT); 
                if(SpuGetKeyStatus(READY) == 3){
                    SpuSetKey(SPU_OFF, READY);
                }
                if (animPlayerQueueSize == 0){
                    if(buttonTimer){
                        short i;
                        for(i = sizeof(aim)/sizeof(frame)-1; i >= 0; i--){
                            *nextFrame = aim[i];
                            nextFrame ++;
                        }
                        *nextFrame = aim[0];
                        nextFrame ++;
                        
                        buttonRActive = 0;
                        buttonTimer = 30;
                    }
                    
                    if(!buttonTimer){

                        short i;
                        for(i = sizeof(aim)/sizeof(frame)-1; i >= 0; i--){
                            *nextFrame = aim[i];
                            nextFrame ++;
                        }
                        for(i = 0; i < sizeof(reload)/sizeof(frame); i++){
                            *nextFrame = reload[i];
                            nextFrame ++;
                        }
                        for(i = sizeof(reload)/sizeof(frame)-1; i >= 3; i--){
                            *nextFrame = reload[i];
                                nextFrame ++;
                        }
                        for(i = sizeof(reload_arrow)/sizeof(frame)-1; i >= 0; i--){
                            *nextFrame = reload_arrow[i];
                                nextFrame ++;
                        }
                        *nextFrame = aim[0];
                        nextFrame ++;
                        
                        buttonRActive = 0;
                        buttonTimer = 30;
                        
                        arrowPoly[0].TransV_L.vy = robinPoly.TransV_L.vy - 12  ;

                        arrowReleased = 1;
                        ammuQty -= 1;


                    }
                }

            }
                 if(!btnRight && !buttonRActive){
                    //~ playSFX(SHOOT); 
                    if(SpuGetKeyStatus(READY) == 3){
                        SpuSetKey(SPU_OFF, READY);
                    }
                }
                

           //~ if(btnLeft && !buttonLActive){   
                    //~ char i;
                    //~ for(i = 0; i < sizeof(reload)/sizeof(frame); i++){
                        //~ *nextFrame = reload[i];
                        //~ nextFrame ++;
                    //~ }
                    //~ for(i = sizeof(reload)/sizeof(frame)-1; i >= 0; i--){
                        //~ *nextFrame = reload[i];
                        //~ nextFrame ++;
                    //~ }
                    //~ buttonLActive = 1;
                    //~ buttonTimer = 30;
        

            //~ }
            
            
            //~ if(!btnLeft && buttonLActive){
                    //~ char i;
                    //~ for(i = sizeof(reload)/sizeof(frame)-1; i >= 0; i--){
                        //~ *nextFrame = reload[i];
                        //~ nextFrame ++;
                    //~ }
                    //~ buttonLActive = 0;
                    //~ buttonTimer = 30;
            //~ }
                
            
            
            if(btnUp && !buttonTimer){
                if( robinPoly.TransV_L.vy > MARGINY * 3){
                        //~ if(SpuGetKeyStatus(JMP) == 0 || SpuGetKeyStatus(JMP) == 3){
                        //~ if (robinPoly.cursor == 0){
                            playSFX(JMP);
                        //~ }
                        //~ }
                        jump[3].Yoffset = -2;
                        char i;
                        for(i = 0; i <= sizeof(jump)/sizeof(frame); i++){
                            *nextFrame = jump[i];
                            nextFrame ++;
                        }

                        buttonUActive = 1;
                        buttonTimer = 30;
                }
            }
            
            
            if(!btnUp){
                buttonUActive = 0;
                //~ SpuSetKey(SPU_OFF, JMP);
        }
            
            if(btnDown && !buttonTimer){
                if( robinPoly.TransV_L.vy < SCREENYRES - robinPoly.height - MARGINY){
                    //~ if(SpuGetKeyStatus(JMP) == 0 || SpuGetKeyStatus(JMP) == 3){
                         playSFX(JMP);
                        //~ }
                        jump[3].Yoffset = +2;
                        char i;
                        for(i = 0; i <= sizeof(jump)/sizeof(frame); i++){
                            *nextFrame = jump[i];
                            nextFrame ++;
                        }


                        buttonDActive = 1;
                        buttonTimer = 30;
                }
            }
            


            if(!btnDown){
                buttonDActive = 0;
                //~ SpuSetKey(SPU_OFF, JMP);

            }

        }
 
            
        //~ if(btnDown && !buttonActive){   
                //~ char i;
                //~ for(i = 0; i < sizeof(jump)/sizeof(frame); i++){
                    //~ *nextFrame = jump[i];
                    //~ nextFrame ++;
                //~ }
                //~ buttonActive = 1;
        //~ }   
        //~ if(!btnUp){
            //~ buttonActive = 0;
        //~ }


        // Objects movements 
        
        if(gameStarted && !gameReset){

            if(!arrowReleased){
                arrowPoly[0].TransV_L.vy += currentFrame->Yoffset ;
                }
            
            if(arrowReleased){
                //~ //arrowPoly.TransV_L.vy = robinPoly.TransV_L.vy + currentFrame->Yoffset ;
                arrowPoly[0].otz = 0;
                arrowPoly[0].TransV_L.vx += ARROW_SPEED;
                if(SpuGetKeyStatus(SHOOT) == 0){
                    playSFX(SHOOT);
                }
                //~ //arrowPoly.TransV_L.vy = arrowPoly.TransV_L.vy;
                
            };
        
            if(arrowPoly[0].TransV_L.vx > SCREENXRES){
                if(SpuGetKeyStatus(SHOOT) == 3){
                    SpuSetKey(SPU_OFF, SHOOT);
                }
                arrowPoly[0].TransV_L.vx = robinPoly.TransV_L.vx - 10 ;
                arrowPoly[0].otz = 2;
                arrowReleased = 0;            
            }
            
            for (int m = assetsLoad.nbrBalloons-1;m >= 0; m--){
                balloonPoly[m].TransV_L.vy -= 1;
                
                if(balloonPoly[m].TransV_L.vy < -balloonPoly[m].height ){
                    balloonPoly[m].TransV_L.vy = SCREENYRES + balloonPoly[m].height ;
                }
            }
            
            
            // Animations 

            // Play frames in animPlayerQueue
            
            animPlayerQueueSize = nextFrame - animPlayerQueue;

            if(robinPoly.timer == 0 ){
                
                if(robinPoly.cursor < 0){
                    robinPoly.cursor = 0;
                }
                currentFrame = &animPlayerQueue[robinPoly.cursor];
                if(robinPoly.cursor < animPlayerQueueSize-1){
                    robinPoly.cursor++;

                    }
                if(robinPoly.cursor == animPlayerQueueSize-1){
                    animPlayerQueue[0] = animPlayerQueue[robinPoly.cursor];
                    robinPoly.cursor = -1;
                    nextFrame = &animPlayerQueue[0];
                    }
                robinPoly.timer = 5;
            }
            robinPoly.timer--;
            

        


            
        }
        
        if(!gameStarted){
            
            parchPoly.otz = 0;
            msgPoly.otz   = 0;
            robinPoly.otz = -1;
            
            //first level 
            
            for (int m = assetsLoad.nbrBalloons-1;m >= 0; m--){
                //~ balloonPoly[m].cursor = 0;
                //~ balloonPoly[m].hit = 0;
                balloonPoly[m].otz = -1;
                //~ balloonPoly[m].TransV_L = (VECTOR){START_POS + (m * 12), SCREENYRES, 0};
                
                }
            
            // second level
            
            
            for (int m = assetsLoad.nbrBubbles-1;m >= 0; m--){
                //~ balloonPoly[m].cursor = 0;
                //~ balloonPoly[m].hit = 0;
                bubblePoly[m].otz = -1;
                beePoly[m].otz = -1;
                //~ balloonPoly[m].TransV_L = (VECTOR){START_POS + (m * 12), SCREENYRES, 0};
                
                }
            
            
            // third level
            //~ targetPoly.otz = -1;
            
            //~ for (int i = 0; i < assetsLoad.nbrArrows; i++){
                //~ arrowPoly[i].otz = -1;
            //~ }
            
            if(msgID == 1 || currentLvl == 0){
                if(!SpuGetKeyStatus(MUSIC)){
                    playSFX(MUSIC);
                }
                SpuSetKey(SPU_OFF, JMP|POP|READY|SHOOT|CROWD);
            }
            if(msgID == 0){
                if(!SpuGetKeyStatus(FOLKS)){
                    playSFX(FOLKS);
                }
                SpuSetKey(SPU_OFF, JMP|POP|READY|SHOOT|CROWD);
            }
            //~ SpuFree(vag_spu_address);
            }
        
        
        if(gameReset){
            
                targetPoly.otz = -1;
                
                for (int i = 0; i < assetsLoad.nbrArrows; i++){
                    arrowPoly[i].otz = -1;
                }
            
                if (currentLvl == 0){
                    menuPoly.otz = -1;
                    //~ instPoly.otz = -1;
                    currentLvl = 1;
                    //~ gameReset = 0;
                    }
                
                if (currentLvl == 1){
               // Level 1
                    nbrBalloonsHit = 0;
                    
                    for (int m = assetsLoad.nbrBalloons-1;m >= 0; m--){
                        balloonPoly[m].cursor = 0;
                        balloonPoly[m].hit = 0;
                        balloonPoly[m].otz = 0;
                        balloonPoly[m].TransV_L = (VECTOR){START_POS + (m * 12), SCREENYRES, 0};
                        srand(rand());
                        balloonPoly[m].TransV_L.vx = rand()/205 + 140;
                        balloonPoly[m].TransV_L.vy = rand()/147;
                        
                    }
                    
                    

                }
                
                if (currentLvl == 2){
                    // Level 2
                    nbrBubblesHit = 0;
                    
                    for (int m = assetsLoad.nbrBubbles-1;m >= 0; m--){
                        bubblePoly[m].cursor = 0;
                        bubblePoly[m].hit = 0;
                        bubblePoly[m].otz = 0;
                        bubblePoly[m].TransV_L.vx = rand()/158 + 80;
                        bubblePoly[m].TransV_L.vy = rand()/145;
                    
                        beePoly[m].cursor = 0;
                        beePoly[m].hit = 0;
                        beePoly[m].otz = 0;
                        beePoly[m].TransV_L.vx = rand()/158 + 80;
                        beePoly[m].TransV_L.vy = rand()/145;

                        srand(rand());
                        bubblePoly[m].TransV_L.vx = rand()/158 + 80;
                        bubblePoly[m].TransV_L.vy = rand()/145;
                    }


                }
                
                if (currentLvl == 3){
                    // Level 3
                    dir = 1;
                    hitId = 1;
                    targetPoly.otz = 0;
                    //~ hitCoord = {0,0,0,0,0};

                }
                
                if(msgID == 0){
                    ammuQty = assetsLoad.ammo;
                }

                
                //UI
                parchPoly.otz = 10;
                msgPoly.otz   = 5;
                robinPoly.otz = 0;
                
                // arrow
                arrowPoly[0].TransV_L.vx = robinPoly.TransV_L.vx - 10 ;
                arrowPoly[0].otz = 2;
                arrowReleased = 0; 
                
                gameStarted = 1;
                gameReset = 0;

                SpuSetKey(SPU_ON, CROWD);
                SpuSetKey(SPU_OFF, JMP|POP|READY|SHOOT|MUSIC|FOLKS );

                //~ FntPrint("reset");
            }
        
        
        
        //~ for(int p = 0; p < START_BALLOONS; p++){
            //~ FntPrint("%d" , balloonPoly[p].hit);
       //~ }
       //~ FntPrint(" %d %d %d %d %d %d\n", currentLvl, btnRight, buttonRActive, gameReset, gameStarted, buttonTimer);
       //~ FntPrint("%d \n", nbrBubblesHit) ;
       //~ FntPrint("%d - SP: %x\n", sync, GetSp()) ;
       //~ FntPrint("%d, %d, %d", bubblePoly[0].poly_ft4->u1, bubblePoly[0].poly_ft4->v1, assetsLoad.nbrBubbles);
        //~ FntPrint("Total size : %dB/512000B", SWAP_ENDIAN32(VAG6fileHeader->dataSize)+SWAP_ENDIAN32(VAGfileHeader->dataSize)+SWAP_ENDIAN32(VAG1fileHeader->dataSize)+SWAP_ENDIAN32(VAG2fileHeader->dataSize)+SWAP_ENDIAN32(VAG3fileHeader->dataSize)+SWAP_ENDIAN32(VAG4fileHeader->dataSize)+SWAP_ENDIAN32(VAG5fileHeader->dataSize));
        //~ FntPrint("\n%d", SpuGetKeyStatus(READY));
        //~ FntPrint("\npitch             : %08x", pitch, (SWAP_ENDIAN32(VAG5fileHeader->samplingFrequency)) );
        //~ FntPrint("\nSet Start addr    : %08x", vag5_spu_address);
        //~ FntPrint("\nReturn start addr : %08x", spu5_start_address);      
        //~ FntPrint("\nGet Start  addr   : %08x", get5_start_addr);  
        //~ FntPrint("\nSend size         : %08x", SWAP_ENDIAN32(VAG5fileHeader->dataSize));  
        //~ FntPrint("\nReturn size       : %08x\n", trans5Size);  
        //~ FntPrint("Ammo : %2d  - Balloons : %2d / %2d\n", ammuQty, nbrBalloonsHit, START_BALLOONS);
        //~ FntPrint("Hold 'O' to ready bow");
        //~ FntPrint("\nRelease to shoot");
        //~ FntPrint("\nBOW&ARROW PSX - Schnappy 2020 ");
        FntFlush(-1);
        
        display();
        }
    return 0;
    }
    
    
// animation system
// https://discord.com/channels/642647820683444236/642849069378568192/781930109800022047
//~ set counter to value, say 5
//~ every iteration of the main update, take away 1
//~ if 0, advance sprite animation to next frame
//~ this counter would be per animated sprite
//~ the update loop would tell every single one that exists to subtract 1
//~ then every object would do said check to advance to the next of their sprites
