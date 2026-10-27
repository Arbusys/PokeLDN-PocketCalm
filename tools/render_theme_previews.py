#!/usr/bin/env python3
"""Create a five-palette contact sheet from the actual firmware renderer."""
from pathlib import Path
from subprocess import run
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
main=root/'firmware/esp32/main';out=root/'previews'
out.mkdir(exist_ok=True)
exe=out/'theme_preview_test'
run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-format-truncation','-I',str(main),
     str(root/'tools/theme_preview.c'),str(main/'screen.c'),str(main/'scene.c'),
     str(main/'tdongle_render.c'),'-lm','-o',str(exe)],check=True)
run([str(exe),str(out)],check=True)
names=['Classic','Modern Minimal','Game Boy Color','Crimson Ruby','Ocean Teal']
views=['home','trade','raid']; w,h=160,80;scale=2
sheet=Image.new('RGB',(1050,5*200+70),(236,234,230));d=ImageDraw.Draw(sheet)
d.text((30,17),'POCKET CALM v0.8 - FIVE FIRMWARE THEMES',fill=(24,40,56))
for i,name in enumerate(names):
    y=60+i*195
    d.text((30,y),f'{i+1:02d}  {name}',fill=(24,40,56))
    for j,view in enumerate(views):
        data=(out/f'theme_{i}_{view}.rgb565').read_bytes()
        assert len(data)==w*h*2
        buf=bytearray()
        for k in range(0,len(data),2):
            n=data[k]*256+data[k+1]
            buf.extend(((n>>11&31)*255//31,(n>>5&63)*255//63,(n&31)*255//31))
        im=Image.frombytes('RGB',(w,h),bytes(buf))
        im.save(out/f'{name.lower().replace(" ","_")}_{view}.png')
        xx=30+j*343;yy=y+24
        sheet.paste(im.resize((w*scale,h*scale),Image.Resampling.NEAREST),(xx,yy))
        d.text((xx+260,y+2),view.upper(),fill=(24,40,56))
    # Check actual variants differ from Classic.
    if i:
        original=(out/f'theme_0_trade.rgb565').read_bytes()
        assert data != (out/f'theme_0_raid.rgb565').read_bytes()
sheet.save(root/'themes_on_device_v08.png')
print('Exported',root/'themes_on_device_v08.png')
