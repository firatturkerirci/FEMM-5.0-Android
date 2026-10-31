# ai_demo_solenoid.py -- an example of the kind of script an AI assistant writes.
#
# Axisymmetric solenoid with a steel plunger. Builds the model from scratch,
# solves it, and prints the coil inductance and the force on the plunger.
# Runs unchanged in FEMM on the desktop (pyFEMM) and in FEMM for Android.

import femm

femm.openfemm()
femm.newdocument(0)                       # 0 = magnetics
femm.mi_probdef(0, "millimeters", "axi", 1e-8, 0, 30)

# Materials from the built-in library
femm.mi_getmaterial("Air")
femm.mi_getmaterial("1010 Steel (hot rolled)")
femm.mi_getmaterial("18 AWG")

# Circuit: 1 A, series
femm.mi_addcircprop("coil", 1.0, 1)

def rect(r1, z1, r2, z2):
    femm.mi_drawrectangle(r1, z1, r2, z2)

# Coil: r 6..16 mm, z -20..20 mm
rect(6, -20, 16, 20)
# Plunger: r 0..5 mm, z -10..30 mm
rect(0, -10, 5, 30)

# Block labels
femm.mi_addblocklabel(11, 0)
femm.mi_selectlabel(11, 0)
femm.mi_setblockprop("18 AWG", 1, 0, "coil", 0, 1, 500)   # 500 turns, group 1
femm.mi_clearselected()

femm.mi_addblocklabel(2.5, 10)
femm.mi_selectlabel(2.5, 10)
femm.mi_setblockprop("1010 Steel (hot rolled)", 1, 0, "<None>", 0, 2, 0)  # group 2 = plunger
femm.mi_clearselected()

femm.mi_addblocklabel(30, 30)
femm.mi_selectlabel(30, 30)
femm.mi_setblockprop("Air", 1, 0, "<None>", 0, 0, 0)
femm.mi_clearselected()

# Open boundary (Kelvin-style ABC shell), radius 100 mm
femm.mi_makeABC(7, 100, 0, 0, 0)
femm.mi_zoomnatural()

femm.mi_saveas("ai_demo_solenoid.fem")
femm.mi_analyze(1)
femm.mi_loadsolution()

i, v, flux = femm.mo_getcircuitproperties("coil")
L = flux / i
femm.mo_groupselectblock(2)
fz = femm.mo_blockintegral(19)            # 19 = z-force (weighted stress tensor)
femm.mo_clearblock()

print("Inductance  L  = %.4f mH" % (L * 1e3))
print("Plunger force Fz = %.4f N" % fz)
