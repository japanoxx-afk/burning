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
