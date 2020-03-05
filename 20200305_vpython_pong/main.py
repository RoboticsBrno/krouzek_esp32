#!/usr/bin/env python3


from vpython import *

# DOKUMENTACE
# intro: https://www.glowscript.org/docs/VPythonDocs/VisualIntro.html
# matematické funkce: https://www.glowscript.org/docs/VPythonDocs/math.html
# box: https://www.glowscript.org/docs/VPythonDocs/box.html
# sphere: https://www.glowscript.org/docs/VPythonDocs/sphere.html

# https://www.glowscript.org/#/user/vbocek/folder/MyPrograms/program/pong

# https://qph.fs.quoracdn.net/main-qimg-db80d2016d3332d8f1cf9fe7b65807df.webp

# Rozměry hřiště - ve skutečnosti polovina
POLE_SIRKA = 24
POLE_VYSKA = 18

# Rozměry hráčů
HRAC_VELIKOST = vector(0.3, 4, 1.5)
PUL_HRACE = HRAC_VELIKOST.y / 2

# Poloměr míčku
BALL_POLOMER = 0.4

# Rychlosti pohybu
RYCHLOST_BALL = 0.6
RYCHLOST_HRAC = 0.3

def uhel_na_vector(stupne):
    radiany = radians(stupne)
    return vector(sin(radiany), cos(radiany), 0)

def vector_na_uhel(vec):
    uhel = asin(vec.x)
    return degrees(uhel)

def kolize(hrac, ball):
    vrchni_okraj = hrac.pos.y + PUL_HRACE
    spodni_okraj = hrac.pos.y - PUL_HRACE

    if ball.pos.y - BALL_POLOMER > vrchni_okraj:
        return False
    if ball.pos.y + BALL_POLOMER < spodni_okraj:
        return False
    return True

# dělící čára uprostřed hřiště. Z souřadnice je -2, aby nekolidovala s míčkem
delici_cara = box(pos=vector(0, 0, -2), size=vector(0.3, POLE_VYSKA * 2, 0.3), color=color.white)

# vytvoření hráčů
hrac_clovek = box (pos=vector(-POLE_SIRKA, 0, 0), size=-HRAC_VELIKOST,  color = color.white)
hrac_ai = box(pos=vector(POLE_SIRKA, 0, 0), size=HRAC_VELIKOST, color=color.red)

# vytvoření míčku
ball = sphere(color=color.green, radius=BALL_POLOMER)

# hmotnost - ovlivňuje pohyb
ball.mass = 1.0

# počáteční pozice je těsně vedle hráče
ball.pos = hrac_clovek.pos + vector(BALL_POLOMER, 0, 0)
ball.p = uhel_na_vector(30 + random()*120)

while True:
    # "vykreselení" snímku každých 33 ms
    rate(33)

    # Pohyb míčku
    ball.pos = ball.pos + (ball.p / ball.mass) * RYCHLOST_BALL

    # Pohyb hráče - počítač
    if hrac_ai.pos.y < ball.pos.y:
        hrac_ai.pos.y += RYCHLOST_HRAC
    else:
        hrac_ai.pos.y -= RYCHLOST_HRAC

    # Pohyb hráče - člověka
    klavesy = keysdown()
    if 'up' in klavesy:
        hrac_clovek.pos.y += RYCHLOST_HRAC
    elif 'down' in klavesy:
        hrac_clovek.pos.y -= RYCHLOST_HRAC

    # Zamezení vyjetí hráčů mimo herní plochu
    for hrac in [hrac_clovek, hrac_ai]:
        if hrac.pos.y + PUL_HRACE > POLE_VYSKA:
            hrac.pos.y = POLE_VYSKA - PUL_HRACE
        elif hrac.pos.y - PUL_HRACE < -POLE_VYSKA:
            hrac.pos.y = -POLE_VYSKA + PUL_HRACE

    # Odraz míčku od horní a spodní hrany
    if ball.pos.y < -POLE_VYSKA or ball.pos.y > POLE_VYSKA:
        ball.p.y = -ball.p.y

    # detekce vyjezí z pole doprava/doleva
    if ball.pos.x < -POLE_SIRKA or ball.pos.x > POLE_SIRKA:

        strana = 1 if ball.pos.x < 0 else -1
        hrac = hrac_clovek if strana == 1 else hrac_ai

        # kolize s hráčem
        if kolize(hrac, ball):
            od_stredu_hrace = (ball.pos.y - hrac.pos.y) / HRAC_VELIKOST.y
            ball.p = uhel_na_vector((90 - 80 * od_stredu_hrace) * strana)
        # gól!
        else:
            ball.pos = hrac.pos + vector(BALL_POLOMER*strana, 0, 0)
            ball.p = uhel_na_vector((30 + random()*120)*strana)


