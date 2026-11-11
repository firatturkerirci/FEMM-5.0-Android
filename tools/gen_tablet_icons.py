#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (C) 2026 Fırat Türkerirci and contributors
#
# gen_tablet_icons.py - draw the tablet toolbar icons (two-tone style) as SVG.
#
# Each icon is drawn on a 24-unit grid: stroked lines plus coloured fills. Two
# sets are written, one for a light and one for a dark theme, into
# gui/icons/tablet/{light,dark}/<name>.svg, named like the classic icon each
# replaces (gui/icons/<name>.png), plus gui/tablet_icons.qrc. The app picks the
# set that matches the system theme in tablet mode; the desktop keeps the
# classic icons. Run it after changing a drawing:
#
#     python3 tools/gen_tablet_icons.py
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def circ(cx, cy, r):
    return f"M{cx-r} {cy}a{r} {r} 0 1 0 {2*r} 0a{r} {r} 0 1 0 {-2*r} 0z"

# Each icon: l = stroked paths, f = [path, opacity, colour-token or None],
# d = dashed stroked paths, png = current FEMM icon
I = {}
def ic(key, en, png, l=(), f=(), d=(), tone='a'):
    I[key] = dict(en=en, png=png, l=list(l), f=[list(x) for x in f], d=list(d), tone=tone)

# Quick actions
ic('script','Script','lua_script',
   l=["M7 3h7l4 4v14H7z","M14 3v4h4","M10.5 12.5 9 14l1.5 1.5","M14.5 12.5 16 14l-1.5 1.5"],
   f=[["M7 3h7l4 4v14H7z",.22]])
ic('mesh','Mesh','mesh',
   l=["M12 4 21 19H3z","M7.5 11.5h9","M7.5 11.5 12 19","M16.5 11.5 12 19"],
   f=[["M12 4 16.5 11.5H7.5z",.45],["M12 19 7.5 11.5h9z",.18]])
ic('solve','Solve','analyze',
   l=["M3 12a9 9 0 1 0 18 0a9 9 0 1 0-18 0z","M10 8.5v7l6-3.5z"],
   f=[["M10 8.5v7l6-3.5z",1]], tone='g')
ic('results','Results','results',
   l=["M9 9h6v6H9z","M9 10.5C5 7 2.5 9 2.5 12s2.5 5 6.5 1.5","M15 10.5c4-3.5 6.5-1.5 6.5 1.5s-2.5 5-6.5 1.5","M9.5 9C8.5 5.5 10 3.5 12 3.5s3.5 2 2.5 5.5","M9.5 15c-1 3.5.5 5.5 2.5 5.5s3.5-2 2.5-5.5"],
   f=[["M9 9h6v6H9z",.6]])
ic('values','Values','pp_point',
   l=["M12 3v4","M12 17v4","M3 12h4","M17 12h4",circ(12,12,3)],
   f=[[circ(12,12,1.4),1]])
ic('contour','Contour','pp_contour',
   l=["M4 18 9 9l5 6 6-10"],
   f=[[circ(4,18,1.9),1],[circ(9,9,1.9),1],[circ(14,15,1.9),1],[circ(20,5,1.9),1]])
ic('area','Area','pp_area',
   d=["M5 8l7-4 7 4-1.5 10L7 19z"],
   f=[["M5 8l7-4 7 4-1.5 10L7 19z",.35]])
ic('plot','Plot','pp_plot',
   l=["M4 4v16h16","M7 15c2.5-7 4.5-7 6.5-2.5S17 17 20 9"],
   f=[["M7 15c2.5-7 4.5-7 6.5-2.5S17 17 20 9v11H7z",.2]])
ic('integrate','Integrate','pp_integrate',
   l=["M14.5 4.5c-1.5-1-3.5-.5-3.5 2v11c0 2.5-2 3-3.5 2"],
   f=[["M15 13.5l5-3V19h-5z",.4]])
ic('fit','Fit','zoomnatural',
   l=["M4 9V4h5","M15 4h5v5","M20 15v5h-5","M9 20H4v-5","M8.5 8.5h7v7h-7z"],
   f=[["M8.5 8.5h7v7h-7z",.3]])
# Drawing
ic('node','Node','node', l=["M12 3.5v4","M12 16.5v4","M3.5 12h4","M16.5 12h4","M9.5 9.5h5v5h-5z"], f=[["M9.5 9.5h5v5h-5z",1]])
ic('segment','Segment','segment', l=["M6.5 17.5 17.5 6.5","M3.5 15.5h4v4h-4z","M16.5 4.5h4v4h-4z"], f=[["M3.5 15.5h4v4h-4z",1],["M16.5 4.5h4v4h-4z",1]])
ic('arc','Arc','arc', l=["M5.5 17.5A13 13 0 0 1 17.5 5.5","M3.5 16.5h4v4h-4z","M16.5 3.5h4v4h-4z"], f=[["M3.5 16.5h4v4h-4z",1],["M16.5 3.5h4v4h-4z",1]])
ic('block','Block label','block', l=["M3.5 12.5 11 5h8.5v8.5L12 21z",circ(15.8,8.7,1.4)], f=[["M3.5 12.5 11 5h8.5v8.5L12 21z",.3]])
ic('group','Group','group', d=["M3.5 3.5h17v17h-17z"], l=["M7 15l3-6 3 6z","M14 9h4v4h-4z"], f=[["M7 15l3-6 3 6z",.5],["M14 9h4v4h-4z",.5]])
ic('move','Move','move', l=["M12 3v18","M3 12h18","M9 6l3-3 3 3","M9 18l3 3 3-3","M6 9l-3 3 3 3","M18 9l3 3-3 3"])
ic('copy','Copy','copy', l=["M8.5 8.5h11v11h-11z","M5 15.5V4.5h11"], f=[["M8.5 8.5h11v11h-11z",.3]])
ic('mirror','Mirror','mirror', d=["M12 3v18"], l=["M9 7 4 17h5z","M15 7l5 10h-5z"], f=[["M9 7 4 17h5z",.6],["M15 7l5 10h-5z",.15]])
ic('delete','Delete','delete', l=["M4 7h16","M9 7V4h6v3","M6 7l1 13h10l1-13","M10 11v6","M14 11v6"], f=[["M6 7l1 13h10l1-13z",.2]], tone='r')
ic('undo','Undo','undo', l=["M9 14 4 9l5-5","M4 9h10.5a5.5 5.5 0 0 1 0 11H11"])
ic('openboundary','Open boundary','openboundary', l=["M12 3a9 9 0 0 1 0 18","M12 6.5a5.5 5.5 0 0 1 0 11","M12 3v18"], f=[["M12 10a2 2 0 0 1 0 4z",1]])
ic('properties','Properties','properties', l=["M4 7h10","M18 7h2","M4 17h4","M12 17h8",circ(16,7,2),circ(10,17,2)], f=[[circ(16,7,2),1],[circ(10,17,2),1]])
# View
ic('zoomin','Zoom in','zoomin', l=[circ(10.5,10.5,6.5),"M15.5 15.5 21 21","M10.5 7.5v6","M7.5 10.5h6"], f=[[circ(10.5,10.5,6.5),.15]])
ic('zoomout','Zoom out','zoomout', l=[circ(10.5,10.5,6.5),"M15.5 15.5 21 21","M7.5 10.5h6"], f=[[circ(10.5,10.5,6.5),.15]])
ic('zoomwindow','Zoom window','zoomwindow', d=["M3.5 3.5h12v12h-12z"], l=["M15.5 15.5 21 21"], f=[["M7 7h5v5H7z",.4]])
ic('showgrid','Show grid','showgrid', f=[[circ(x,y,1.25),1] for y in (6,12,18) for x in (6,12,18)])
ic('snapgrid','Snap to grid','snapgrid', l=["M9.5 9.5h5v5h-5z"], f=[[circ(x,y,1.1),.45] for y in (6,12,18) for x in (6,12,18) if (x,y)!=(12,12)]+[["M9.5 9.5h5v5h-5z",1]])
# Results display
ic('dplot','Density plot','pp_dplot', l=["M4 4h16v16H4z","M12 4v16","M4 12h16"],
   f=[["M4 4h8v8H4z",1,'c4'],["M12 4h8v8h-8z",1,'c3'],["M4 12h8v8H4z",1,'c2'],["M12 12h8v8h-8z",1,'c1']])
ic('cplot','Flux lines','pp_cplot', l=["M3 18c5-10 13-10 18 0","M6.5 18c3-6 8-6 11 0","M10 18c.8-2.3 3.2-2.3 4 0","M2.5 20.5h19"])
ic('vplot','Vectors','pp_vplot', l=["M4 19l6-6","M7.5 13H10v2.5","M11 13l6-6","M14.5 7H17v2.5","M13 19l6-6","M16.5 13H19v2.5"])
ic('circprops','Circuit','pp_circprops', l=["M2.5 12h3","M18.5 12h3","M5.5 12c0-4.5 3.25-4.5 3.25 0s3.25 4.5 3.25 0 3.25-4.5 3.25 0 3.25 4.5 3.25 0"])
# Script panel (no FEMM icons today: text buttons)
ic('run','Run',None, l=["M8 5.5v13l10.5-6.5z"], f=[["M8 5.5v13l10.5-6.5z",1]], tone='g')
ic('stop','Stop',None, l=["M7 7h10v10H7z"], f=[["M7 7h10v10H7z",1]], tone='r')
ic('paste','Paste',None, l=["M9 3.5h6v3H9z","M8 5H6v15.5h12V5h-2","M9 12h6","M9 15.5h4"], f=[["M6 5h12v15.5H6z",.2]])
ic('open','Open','open_file', l=["M3 19V5h6l2 2h10v12z","M3 9.5h18"], f=[["M3 9.5h18V19H3z",.25]])
ic('save','Save',None, l=["M12 4v11","M7.5 10.5 12 15l4.5-4.5","M4 17v3h16v-3"])
ic('copyerror','Copy error',None, l=["M9 3.5h6v3H9z","M8 5H6v15.5h12V5h-2","M12 10v4.5","M12 17.2v.3"], f=[["M6 5h12v15.5H6z",.2]], tone='w')
ic('aiprompt','AI prompt',None, l=["M4 5h16v11H9.5L4 20z"], f=[["M13 6.8l.9 2.2 2.2.9-2.2.9-.9 2.2-.9-2.2-2.2-.9 2.2-.9z",1]])


# Toolbar buttons that the concept page did not cover.
ic('new','New','new', l=["M7 3h7l4 4v14H7z","M14 3v4h4","M12.5 11v6","M9.5 14h6"], f=[["M7 3h7l4 4v14H7z",.22]])
ic('panup','Pan up','panup', l=["M12 20V5","M6 11l6-6 6 6"])
ic('pandown','Pan down','pandown', l=["M12 4v15","M6 13l6 6 6-6"])
ic('panleft','Pan left','panleft', l=["M20 12H5","M11 6l-6 6 6 6"])
ic('panright','Pan right','panright', l=["M4 12h15","M13 6l6 6-6 6"])
ic('rectselect','Select window','rectselect', d=["M3.5 3.5h13v13h-13z"], l=["M12 12l8 3-3.5 1.2L15 20z"], f=[["M12 12l8 3-3.5 1.2L15 20z",1]])
ic('circselect','Select circle','circselect', d=[circ(10,10,6.5)], l=["M12 12l8 3-3.5 1.2L15 20z"], f=[["M12 12l8 3-3.5 1.2L15 20z",1]])
ic('scale','Scale','scale', d=["M4 4h16v16H4z"], l=["M4 13h7v7H4z","M14 10l5-5","M15 5h4v4"], f=[["M4 13h7v7H4z",.5]])
ic('gridsize','Grid size','gridsize', l=["M4 9h16","M4 15h16","M9 4v16","M15 4v16"], f=[["M9 9h6v6H9z",.4]])
ic('createradius','Create radius','createradius', l=["M5 20v-7a8 8 0 0 1 8-8h7"], d=["M5 5v8M5 5h8"], f=[[circ(5,20,1.6),1],[circ(20,5,1.6),1]])
ic('showmesh','Show mesh','pp_showmesh', l=["M12 4 21 19H3z","M7.5 11.5h9","M7.5 11.5 12 19","M16.5 11.5 12 19"], f=[["M12 4 16.5 11.5H7.5z",.3]])
ic('condprops','Conductor','pp_condprops', l=["M3 12h4","M17 12h4","M7 8h10v8H7z"], f=[["M7 8h10v8H7z",.5]])
ic('console','Python console','lua_console', l=["M3 5h18v14H3z","M7 10l3 2-3 2","M12 15h5"], f=[["M3 5h18v14H3z",.2]])
ic('stamp','Stamp','stamp', l=["M9 4h6v6l3 3v3H6v-3l3-3z","M5 20h14"], f=[["M6 13h12v3H6z",.5]])

# Dock title bar buttons (tablet): move the panel to a side, or close it.
ic('dockleft','Dock left',None, l=["M3.5 4.5h17v15h-17z","M9.5 4.5v15"], f=[["M3.5 4.5h6v15h-6z",.6]])
ic('dockright','Dock right',None, l=["M3.5 4.5h17v15h-17z","M14.5 4.5v15"], f=[["M14.5 4.5h6v15h-6z",.6]])
ic('dockbottom','Dock bottom',None, l=["M3.5 4.5h17v15h-17z","M3.5 14h17"], f=[["M3.5 14h17v5.5h-17z",.6]])
ic('close','Close',None, l=["M6.5 6.5l11 11","M17.5 6.5l-11 11"])

TONES = {
    'light': dict(fg='#16232a', a='#0b7f86', g='#23874b', r='#c0392b', w='#b7791f'),
    'dark':  dict(fg='#e3eaed', a='#3cc4cb', g='#4cc77a', r='#ef6b5d', w='#e3a948'),
}
DENSITY = dict(c1='#22c8e0', c2='#4cc75a', c3='#f2c718', c4='#e0309a')   # FEMM's scale

def svg(icon, t):
    tone = t[icon['tone']] if icon['tone'] in t else t['a']
    parts = []
    for fill in icon['f']:
        d, op = fill[0], fill[1]
        c = fill[2] if len(fill) > 2 else None
        colour = DENSITY[c] if c else tone
        opacity = 1 if c else min(1, max(0.42, op * 1.6))
        parts.append(f'<path d="{d}" fill="{colour}" fill-opacity="{opacity:g}"/>')
    common = f'fill="none" stroke="{t["fg"]}" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"'
    for d in icon['l']:
        parts.append(f'<path d="{d}" {common}/>')
    for d in icon['d']:
        parts.append(f'<path d="{d}" {common} stroke-dasharray="2.2 2.2"/>')
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 24 24">'
            + ''.join(parts) + '</svg>\n')

# File name = the classic icon it replaces, or the key for icons with no
# classic counterpart (the script panel's buttons).
written = []
for theme, t in TONES.items():
    outdir = os.path.join(ROOT, 'gui', 'icons', 'tablet', theme)
    os.makedirs(outdir, exist_ok=True)
    for key, icon in I.items():
        name = icon['png'] or key
        with open(os.path.join(outdir, name + '.svg'), 'w', encoding='utf-8') as f:
            f.write(svg(icon, t))
        written.append(f'icons/tablet/{theme}/{name}.svg')

with open(os.path.join(ROOT, 'gui', 'tablet_icons.qrc'), 'w', encoding='utf-8') as f:
    f.write('<RCC>\n  <qresource prefix="/tablet">\n')
    for w in sorted(written):
        alias = w[len('icons/tablet/'):]
        f.write(f'    <file alias="{alias}">{w}</file>\n')
    f.write('  </qresource>\n</RCC>\n')
print(len(written), 'files')
