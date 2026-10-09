#pragma once
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "sc_engine.h"
#include "sc_addresses.h"
#include "sc_console.h"

// Draw into the final locked primary only: no game state or mouse region changes.
// BWAPI BWGame: base 0x57F0F0 + elapsedTime offset 0xE608 (engine seconds).
static void BgClockDraw(BYTE* pixels, DWORD pitch, int width, int height) {
    if (ScConsoleInGame()!=1 || !pixels || width<640 || height<200) return;
    const BYTE* pal=(const BYTE*)ScRuntimeAddr(SC_VA_PAL_WRITTEN);
    if (!ScReadableAt(pal,1024)) return;
    BYTE ink=0, shadow=0; int bright=-1, dark=1000;
    for(int i=0;i<256;i++) {
        int v=pal[i*4]+pal[i*4+1]+pal[i*4+2];
        if(v>bright){bright=v;ink=(BYTE)i;}
        if(v<dark){dark=v;shadow=(BYTE)i;}
    }
    static const BYTE digits[11][7]={
      {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
      {14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
      {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
      {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
      {14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
      {0,4,4,0,4,4,0}};
    if (!ScReadable(0x0058D6F8u,4)) return;
    unsigned t=*(DWORD*)ScRuntimeAddr(0x0058D6F8u);
    char text[32]; snprintf(text,sizeof(text),"%02u:%02u",t/60,t%60);
    int x0=width/2+306-(int)strlen(text)*12, y0=height-196;
    for(int pass=0;pass<2;pass++) for(int c=0;text[c];c++) {
        int n=text[c]==':'?10:text[c]-'0';
        for(int y=0;y<7;y++)for(int x=0;x<5;x++)if(digits[n][y]&(1<<(4-x)))
          for(int sy=0;sy<2;sy++)for(int sx=0;sx<2;sx++) {
            int px=x0+c*12+x*2+sx+(pass==0), py=y0+y*2+sy+(pass==0);
            if(px>=0&&px<width&&py>=0&&py<height) pixels[py*pitch+px]=pass?ink:shadow;
          }
    }
}
