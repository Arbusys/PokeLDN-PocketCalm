#!/usr/bin/env python3
"""Compile the actual production OLED128 renderer with test state and save real pixel screenshots."""
from pathlib import Path
from subprocess import run
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
main = root / 'firmware' / 'esp32' / 'main'
out = root / 'previews' / 'oled128'
out.mkdir(parents=True, exist_ok=True)
exe = out / 'render_oled_host'
run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-O2',
     '-I',str(main),str(root/'tools'/'oled128_preview.c'),
     str(main/'pocket_oled_render.c'),str(main/'screen.c'),
     '-o',str(exe)],check=True)
scenes=['home','trade','raid','received']
for scene in scenes:
    pgm=out/(scene+'.pgm')
    run([str(exe),scene,str(pgm)],check=True)
    im=Image.open(pgm).convert('RGB')
    # Pixel accurate enlargement; no interpolating artifacts.
    im=im.resize((128*4,64*4),Image.Resampling.NEAREST)
    im.save(out/(scene+'.png'))

bg=(10,19,32); fg=(234,239,231); panel=(19,38,55); accent=(111,189,166)
canvas=Image.new('RGB',(1220,895),bg); d=ImageDraw.Draw(canvas)
fontfile='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
fontbold='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
titlefont=ImageFont.truetype(fontbold,33); headfont=ImageFont.truetype(fontbold,22); smallfont=ImageFont.truetype(fontfile,16)
d.text((65,31),'POCKET CALM  •  OLED 128 × 64',font=titlefont,fill=fg)
d.text((65,79),'Andrew SSD1309 TEST — pixels from the firmware C renderer',font=smallfont,fill=accent)
for i, (scene,label,desc) in enumerate([
    ('home','01  •  HOME','Radio + connection status'),
    ('trade','02  •  TRADE','Two sprites, moving Poké Ball'),
    ('raid','03  •  RAID','Boss, rank, players, link code'),
    ('received','04  •  RECEIVED','Transfer confirmation')]):
    col=i%2; row=i//2; x=60+col*600; y=123+row*367
    d.rounded_rectangle((x,y,x+560,y+344),radius=20,fill=panel,outline=(48,80,99),width=2)
    d.text((x+24,y+16),label,font=headfont,fill=fg)
    d.text((x+24,y+51),desc,font=smallfont,fill=(151,179,190))
    display=Image.open(out/(scene+'.png')).convert('RGB')
    canvas.paste(display,(x+24,y+80))
    d.rounded_rectangle((x+19,y+75,x+24+512+5,y+80+256+5),radius=5,outline=(96,143,157),width=2)
d.text((60,865),'Preview from C renderer / fictitious events & silhouettes; hardware not yet tested.',font=smallfont,fill=(174,196,199))
canvas.save(out/'PocketCalm-OLED128-overview.png')
print('Wrote',out)

# The animation preview is sampled frame-by-frame from the very same C
# renderer the ESP32 uses, not a concept illustration.
frames=[]
for elapsed in range(0,3000,150):
    pgm=out/f'trade_{elapsed:04d}.pgm'
    run([str(exe),'trade',str(pgm),str(elapsed)],check=True)
    frames.append(Image.open(pgm).convert('P').resize((128*4,64*4),Image.Resampling.NEAREST))
frames[0].save(out/'trade-animation-preview.gif',save_all=True,append_images=frames[1:],duration=150,loop=0,optimize=False)
for pgm in out.glob('*.pgm'): pgm.unlink()
