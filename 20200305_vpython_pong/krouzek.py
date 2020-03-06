

from vpython import *

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

delici_cara = box(pos=vector(0, 0, -2),size=vector(0.3, POLE_VYSKA * 2, 0.3),color=color.white)


hrac_clovek = box(pos=vector(-POLE_SIRKA, 0, 0), size=HRAC_VELIKOST, color=color.white)
hrac_ai = box(pos=vector(POLE_SIRKA, 0, 0), size=HRAC_VELIKOST, color=color.red)


ball = sphere(color=color.green, radius=BALL_POLOMER)
ball.pos = hrac_clovek.pos + vector(0.5, 0, 0)

ball.p = vector(0.7, 0.2, 0)

while True:
    rate(33) # 33ms

    ball.pos += ball.p * RYCHLOST_BALL

    # Odražení v X souřadnici bude potřeba upravit, aby fungovalo jen když
    # se ball srazí s hráčem
    if ball.pos.x > POLE_SIRKA or ball.pos.x < -POLE_SIRKA:
        ball.p.x = -ball.p.x
    if ball.pos.y > POLE_VYSKA or ball.pos.y < -POLE_VYSKA:
        ball.p.y = -ball.p.y

    klavesy = keysdown()
    # [ "up", "down" ]
    if "up" in klavesy:
        hrac_clovek.pos.y += RYCHLOST_HRAC
    elif "down" in klavesy:
        hrac_clovek.pos.y -= RYCHLOST_HRAC

    for h in [ hrac_clovek, hrac_ai ]:
        if h.pos.y + PUL_HRACE > POLE_VYSKA:
            h.pos.y = POLE_VYSKA - PUL_HRACE
        elif h.pos.y - PUL_HRACE < -POLE_VYSKA:
            h.pos.y = -POLE_VYSKA + PUL_HRACE
