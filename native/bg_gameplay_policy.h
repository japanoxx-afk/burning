#pragma once
#include <stdint.h>

// units.dat properties, CUnit status and order IDs from StarCraft 1.16.1.
static inline bool BgSelectable(unsigned id,uint32_t properties,uint32_t status,unsigned order,bool idle){
    if(!(status&1)||(status&0x1068))return false;
    bool worker=id==7||id==41||id==64;
    if(idle)return worker&&(order==1||order==2||order==3);
    return !worker&&!(properties&0x0080281b)&&(properties&0x18208000);
}
static inline uint32_t BgCrusaderSpeed(uint32_t base){return (uint32_t)((uint64_t)base*4/5);}

struct BgBounds { uint16_t left,top,right,bottom; };
struct BgExitPoint { int x,y; };
static inline int BgClamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static inline uint64_t BgExitDistance(BgExitPoint p,int x,int y){
    int64_t dx=p.x-x,dy=p.y-y;return (uint64_t)(dx*dx+dy*dy);
}
// Rank nearby exits by distance to the rally. The rally itself is never a spawn point.
static inline void BgExitCandidates(int x,int y,BgBounds building,BgBounds unit,int rx,int ry,BgExitPoint out[40]){
    int l=x-building.left-unit.right-8,r=x+building.right+unit.left+8;
    int t=y-building.top-unit.bottom-8,b=y+building.bottom+unit.top+8;
    out[0]={l,BgClamp(ry,t,b)};out[1]={r,BgClamp(ry,t,b)};
    out[2]={BgClamp(rx,l,r),t};out[3]={BgClamp(rx,l,r),b};
    for(int i=0;i<9;i++){
        int px=l+(r-l)*i/8,py=t+(b-t)*i/8;
        out[4+i*4]={l,py};out[5+i*4]={r,py};out[6+i*4]={px,t};out[7+i*4]={px,b};
    }
    for(int i=1;i<40;i++){
        BgExitPoint p=out[i];int j=i;
        while(j&&BgExitDistance(p,rx,ry)<BgExitDistance(out[j-1],rx,ry)){out[j]=out[j-1];--j;}
        out[j]=p;
    }
}
