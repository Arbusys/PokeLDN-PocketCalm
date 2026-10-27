/* Native graphics + rear-button selector regression tests (no ESP-IDF needed).
   Uses real scene and render code; only bitmap content is synthetic. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "screen.h"
#include "tdongle_render.h"

static uint8_t frame[SCREEN_BYTES];
static uint8_t pixels[TDONGLE_PIXEL_BYTES];
static void save(const char *file, const scene_radio_t *state, uint32_t at)
{
    scene_visual_t view;
    scene_draw(frame, state, at);
    scene_visual(&view, at);
    tdongle_render(frame, pixels, &view, at);
    FILE *out = fopen(file,"wb");
    if (!out) exit(5);
    if (fwrite(pixels,1,sizeof(pixels),out)!=sizeof(pixels)) exit(6);
    fclose(out);
}
static void fake_sprite(uint8_t slot)
{
    uint8_t p[4+5*38] = {DISPLAY_OP_SPRITE,slot,40,38};
    for (int y=0;y<38;++y)
        for (int x=0;x<40;++x) {
            int dx=x-20,dy=y-20;
            int head=dx*dx/230 + dy*dy/300<=1;
            int ear=y<17&&((x>6&&x<14)||(x>26&&x<34));
            int eye=y>=16&&y<=19&&((x>=13&&x<=15)||(x>=24&&x<=26));
            if ((head||ear)&&!eye) p[4+y*5+x/8] |= 0x80u>>(x%8);
        }
    if (!scene_command(p,sizeof(p),0)) exit(7);
}
static void fake_trade(void)
{
    scene_reset();fake_sprite(SLOT_OURS);fake_sprite(SLOT_THEIRS);
    const uint8_t p[]={DISPLAY_OP_SHOW,SHOW_TRADE,0,0,'S','w','/','S','h',0,'E','E','V','E','E',0};
    if (!scene_command(p,sizeof(p),100)) exit(8);
}
static void fake_raid(void)
{
    scene_reset();fake_sprite(SLOT_OURS);
    const uint8_t p[]={DISPLAY_OP_LOBBY,5,2,3,'M','a','x',' ','R','a','i','d',0,
      'P','i','k','a','c','h','u',0,'1','2','3','4',0};
    if (!scene_command(p,sizeof(p),100)) exit(9);
}
int main(int argc,char**argv)
{
    if(argc!=2) return 2;
    const scene_radio_t state={.mode=SCENE_HOSTING,.stations=1};
    if(tdongle_theme_count()!=5u) return 3;
    tdongle_theme_set(0);
    for (unsigned i=0;i<5;i++) {
        if(tdongle_theme_get()!=i) return 4;
        char file[300];
        scene_reset();
        snprintf(file,sizeof(file),"%s/theme_%u_home.rgb565",argv[1],i);
        save(file,&state,5000);
        fake_trade();
        snprintf(file,sizeof(file),"%s/theme_%u_trade.rgb565",argv[1],i);
        save(file,&state,5000);
        fake_raid();
        snprintf(file,sizeof(file),"%s/theme_%u_raid.rgb565",argv[1],i);
        save(file,&state,5000);
        tdongle_theme_next(6000+i*1000);
    }
    if(tdongle_theme_get()!=0u) return 10;
    tdongle_theme_next(12000);
    if(tdongle_theme_get()!=1u || tdongle_theme_changed_ms()!=12000u) return 11;
    printf("PASS: 5 themes, rear-button cycling, wrap-around, 15 native frames\n");
    return 0;
}
