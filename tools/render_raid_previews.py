#!/usr/bin/env python3
"""Compile the actual C renderer and create a readable 5-screen raid contact sheet."""
from pathlib import Path
from subprocess import run
import sys
from PIL import Image,ImageOps,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
main=root/'firmware/esp32/main'
out=root/'previews'
out.mkdir(exist_ok=True)
exe=out/'raid_preview_test'
run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-format-truncation','-I',str(main),
    str(root/'tools/raid_preview.c'),str(main/'screen.c'),str(main/'scene.c'),
    str(main/'tdongle_render.c'),'-lm','-o',str(exe)],check=True)
run([str(exe),str(out)],check=True)
labels=['TERA RAID','MAX RAID','GMAX RAID','DYNAMAX ADV','UNKNOWN-TITLE FALLBACK','GUESTS UPDATE','TRADE REGRESSION']
frames=[]
for i,name in enumerate(['raid_0','raid_1','raid_2','raid_3','raid_4','joined','trade']):
    data=(out/f'{name}.rgb565').read_bytes()
    assert len(data)==160*80*2
    rgb=bytearray()
    for p in range(0,len(data),2):
        n=data[p]*256+data[p+1]
        rgb.extend(((n>>11 & 31)*255//31,(n>>5 & 63)*255//63,(n & 31)*255//31))
    frame=Image.frombytes('RGB',(160,80),bytes(rgb))
    frame.save(out/f'{name}.png')
    scaled=frame.resize((640,320),Image.Resampling.NEAREST)
    frames.append(scaled)
w,h=640, (320+47)*len(frames)+16
sheet=Image.new('RGB',(w+36,h),'#EEE9DF')
draw=ImageDraw.Draw(sheet)
for i,(label,fr) in enumerate(zip(labels,frames)):
    y=12+i*367
    draw.text((18,y),label,fill='#142B46')
    sheet.paste(fr,(18,y+22))
sheet.save(root/'raid_screens_v07.png')
print('Verified 7 renderer scenarios and exported',root/'raid_screens_v07.png')
