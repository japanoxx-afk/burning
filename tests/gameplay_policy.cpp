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
    BgBounds building={64,48,64,48},unit={8,8,8,8};BgExitPoint exits[40];
    for(BgExitPoint rally: {BgExitPoint{4000,1000},BgExitPoint{0,1000},BgExitPoint{1000,0},BgExitPoint{1000,4000},BgExitPoint{4000,4000}}){
        BgExitCandidates(1000,1000,building,unit,rally.x,rally.y,exits);
        for(int i=0;i<40;i++){
            assert(exits[i].x>=920&&exits[i].x<=1080&&exits[i].y>=936&&exits[i].y<=1064);
            assert(exits[i].x==920||exits[i].x==1080||exits[i].y==936||exits[i].y==1064);
            if(i)assert(BgExitDistance(exits[i-1],rally.x,rally.y)<=BgExitDistance(exits[i],rally.x,rally.y));
        }
        if(rally.x==4000)assert(exits[0].x==1080);
        if(rally.x==0)assert(exits[0].x==920);
        if(rally.y==0)assert(exits[0].y==936);
        if(rally.y==4000)assert(exits[0].y==1064);
    }
    // Edge-of-map candidates remain signed so the caller rejects off-map exits.
    BgExitCandidates(20,20,building,unit,0,20,exits);assert(exits[0].x<0);
}
