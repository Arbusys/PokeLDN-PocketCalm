/* Host-side exact pixel rendering preview of pocket_oled_render.c, 128x64.
   Uses synthetic test silhouettes and mock events (no physical device). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "scene.h"
#include "screen.h"
#include "pocket_oled_render.h"

static uint8_t demo_bits[3][32 * 32 / 8];
static const char *demo_names[] = {"SAMPLE A", "SAMPLE B", "SAMPLE C"};
static int scenario;

static void setpixel(int sprite, int x, int y)
{
    if (x < 0 || y < 0 || x >= 32 || y >= 32) return;
    demo_bits[sprite][y * 4 + x / 8] |= 0x80u >> (x % 8);
}
static void seed_sprites(void)
{
    /* An original demo creature made of ears, feet and tail; deliberately
       synthetic, not copyrighted artwork or a purported Pokémon sprite. */
    for (int y=7; y<26; y++) for (int x=5; x<27; x++) {
        int dx=x-16,dy=y-18;
        if (dx*dx*3+dy*dy*4<280) setpixel(0,x,y);
    }
    for(int y=1;y<12;y++) for(int x=7;x<13;x++) if (x+y>10 && x-y<10) setpixel(0,x,y);
    for(int y=1;y<12;y++) for(int x=20;x<26;x++) if(x-y>15) setpixel(0,x,y);
    for(int x=7;x<13;x++) for(int y=25;y<31;y++) setpixel(0,x,y);
    for(int x=20;x<26;x++) for(int y=25;y<31;y++) setpixel(0,x,y);
    for(int y=0;y<9;y++) setpixel(0,31-y/2,y+13);
    for(int y=14;y<18;y++) {setpixel(0,11,y);setpixel(0,21,y);}
    for(int x=14;x<19;x++) setpixel(0,x,21);
    /* Another distinct abstract star-like sprite. */
    for(int y=5;y<29;y++) for(int x=5;x<29;x++) {
        int dx=x-16,dy=y-16;
        if(dx*dx+dy*dy<125) setpixel(1,x,y);
    }
    for(int i=0;i<7;i++) {
        setpixel(1,5+i,2+i);setpixel(1,26-i,2+i);
        setpixel(1,1+i,21+i);setpixel(1,30-i,21+i);
    }
    for(int y=14;y<19;y++){setpixel(1,12,y);setpixel(1,20,y);}
    for(int x=13;x<20;x++)setpixel(1,x,23);
    memcpy(demo_bits[2],demo_bits[1],sizeof demo_bits[2]);
}

const char *scene_label(void){return scenario==2?"SAMPLE BOSS":scenario==3?"SAMPLE B":"";}
const char *scene_title(void){return scenario==2?"Tera raid":"Trade";}
const char *scene_lobby_code(void){return scenario==2?"7421": "";}
uint8_t scene_lobby_joined(void){return 2;}
uint8_t scene_lobby_seats(void){return 4;}
uint8_t scene_lobby_stars(void){return 5;}
const char *scene_sprite_name(uint8_t slot){return slot<3?demo_names[slot]:"";}
const uint8_t *scene_sprite_bits(uint8_t slot,uint8_t *w,uint8_t *h)
{
    if (w) *w = 0;
    if (h) *h = 0;
    if(slot>2) return NULL;
    if (w) *w = 32;
    if (h) *h = 32;
    return demo_bits[slot];
}

int main(int argc, char **argv)
{
    if(argc<3 || argc>4){fprintf(stderr,"usage: oled128_preview SCENE OUTPUT.pgm [elapsed_ms]\nSCENE: home trade raid received\n");return 2;}
    if(!strcmp(argv[1],"home"))scenario=0;
    else if(!strcmp(argv[1],"trade"))scenario=1;
    else if(!strcmp(argv[1],"raid"))scenario=2;
    else if(!strcmp(argv[1],"received"))scenario=3;
    else {fprintf(stderr,"Unknown scene\n");return 2;}
    seed_sprites();
    uint8_t fb[SCREEN_BYTES]={0};
    scene_radio_t radio={.mode=SCENE_JOINED,.rx=123,.tx=94};
    scene_visual_t v={.show= scenario==0?SHOW_AUTO:scenario==1?SHOW_TRADED:scenario==2?SHOW_LOBBY:SHOW_TRADED,
                      .radio_mode=SCENE_JOINED,.elapsed_ms=scenario==3?5200:1500,.reveal_ms=3000};
    if (argc == 4 && scenario == 1) v.elapsed_ms = (uint32_t)strtoul(argv[3], NULL, 10);
    pocket_oled_render(fb,&radio,&v,v.elapsed_ms);
    FILE *out=fopen(argv[2],"wb");if(!out){perror(argv[2]);return 1;}
    fprintf(out,"P5\n%d %d\n255\n",SCREEN_W,SCREEN_H);
    for(int y=0;y<SCREEN_H;y++)for(int x=0;x<SCREEN_W;x++){
        const uint8_t c=fb_get(fb,x,y)?255:0;
        if(fwrite(&c,1,1,out)!=1){fclose(out);return 1;}
    }
    fclose(out);return 0;
}
