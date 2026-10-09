#include <assert.h>
#include <initializer_list>
#include "../native/bg_gameplay_policy.h"
int main(){
    for(unsigned worker:{7u,41u,64u}){
        assert(BgSelectable(worker,0x58010008,1,3,true));
        for(unsigned order:{6u,0x55u,0x57u,0x5au,0x21u})assert(!BgSelectable(worker,0x58010008,1,order,true));
        assert(!BgSelectable(worker,0x58010008,1,3,false));
    }
    assert(BgSelectable(0,0x18010000,1,3,false)); // armed organic
    assert(BgSelectable(5,0x58000000,1,3,false)); // armed mechanical; organic bit is absent
    assert(BgSelectable(34,0x00210000,1,3,false)); // spellcaster support
    assert(!BgSelectable(106,0x98000001,1,3,false)); // armed building
    assert(!BgSelectable(4,0x18000010,1,3,false)); // turret subunit
    assert(!BgSelectable(176,0x80002000,1,3,false)); // resource
    assert(!BgSelectable(89,0x00010000,1,3,false)); // critter
    for(unsigned status:{0u,0x21u,0x41u,0x1001u})assert(!BgSelectable(0,0x18010000,status,3,false));
    assert(BgCrusaderSpeed(5*256)==4*256);
    assert(BgCrusaderSpeed(0)==0);
    assert(BgCrusaderSpeed(0xffffffff)==3435973836u);
}
