#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../app/src/identification_scan.c"
static IdentificationScan configured(void) {
    IdentificationScan s={.configured=0x3ff,.config={-.5f,.5f,0,.1f,.2f,2,70,2,.02f,120}};
    return s;
}
int main(void) {
    IdentificationScan s=configured();
    assert(identification_scan_start(&s,42,0,0,0,true));
    float q=0,v=0;
    unsigned forward=0,reverse=0;
    for (unsigned tick=20;tick<2400000 && s.active;tick+=20) {
        assert(identification_scan_step(&s,tick,q,v,.1f,25,true,true));
        if (s.phase==3)++forward;
        if (s.phase==4)++reverse;
        assert(fabsf(s.velocity-v)<=.00021f);
        v=s.velocity;q+=v*.001f;
        assert(q>s.config[0] && q<s.config[1]);
    }
    assert(!s.active && s.phase==6 && s.legs==4 && forward && reverse && fabsf(q)<.001f);
    for (unsigned reason=1;reason<=7;++reason) {
        s=configured();assert(identification_scan_start(&s,43,0,0,0,true));s.velocity=.1f;
        identification_scan_abort(&s,(uint8_t)reason);
        for(unsigned tick=20;tick<40000 && s.active;tick+=20)
            identification_scan_step(&s,tick,0,0,0,25,true,true);
        assert(s.phase==7 && !s.active && s.velocity==0 && s.reason==reason);
    }
    s=configured();assert(identification_scan_start(&s,44,0,0,0,true));
    identification_scan_step(&s,20,0,0,0,25,true,false);assert(s.reason==3);
    s=configured();s.config[2]=.49f;assert(!identification_scan_start(&s,1,0,.49f,0,true));
    s=configured();s.config[7]=1.5f;assert(!identification_scan_start(&s,1,0,0,0,true));
    s=configured();assert(!identification_scan_start(&s,1,0,0,0,false));
    s=configured();assert(identification_scan_start(&s,1,0,0,0,true));
    identification_scan_step(&s,20,.51f,0,0,25,true,true);assert(s.reason==4 && s.velocity==0);
    puts("scan trajectory, bounds, repeats, return, slew and abort tests passed");
}
