# ai_demo_capacitor_sweep.py -- a parameter sweep, the other common kind of
# AI-written script. Planar parallel-plate capacitor with a dielectric slab;
# sweeps the plate gap, prints the capacitance per metre of depth and writes a
# CSV table.

import femm

femm.openfemm()

rows = []
for gap in [1.0, 2.0, 3.0, 4.0]:          # mm
    femm.newdocument(1)                    # 1 = electrostatics
    femm.ei_probdef("millimeters", "planar", 1e-8, 1000, 30)   # depth 1 m

    femm.ei_addmaterial("Air", 1, 1, 0)
    femm.ei_addmaterial("FR4", 4.4, 4.4, 0)
    femm.ei_addconductorprop("plus", 100, 0, 1)     # 100 V
    femm.ei_addconductorprop("gnd", 0, 0, 1)

    w = 20.0
    t = 0.5
    # bottom plate, dielectric, top plate
    femm.ei_drawrectangle(-w / 2, -t, w / 2, 0)
    femm.ei_drawrectangle(-w / 2, gap, w / 2, gap + t)

    # plate edges at fixed voltage
    femm.ei_selectsegment(0, -t)
    femm.ei_selectsegment(0, 0)
    femm.ei_selectsegment(-w / 2, -t / 2)
    femm.ei_selectsegment(w / 2, -t / 2)
    femm.ei_setsegmentprop("<None>", 0, 1, 0, 0, "gnd")
    femm.ei_clearselected()
    femm.ei_selectsegment(0, gap)
    femm.ei_selectsegment(0, gap + t)
    femm.ei_selectsegment(-w / 2, gap + t / 2)
    femm.ei_selectsegment(w / 2, gap + t / 2)
    femm.ei_setsegmentprop("<None>", 0, 1, 0, 0, "plus")
    femm.ei_clearselected()

    # the plates are conductors at a fixed voltage: no mesh inside them
    for yc in (-t / 2, gap + t / 2):
        femm.ei_addblocklabel(0, yc)
        femm.ei_selectlabel(0, yc)
        femm.ei_setblockprop("<No Mesh>", 1, 0, 0)
        femm.ei_clearselected()

    # dielectric between the plates
    femm.ei_drawline(-w / 2, 0, -w / 2, gap)
    femm.ei_drawline(w / 2, 0, w / 2, gap)
    femm.ei_addblocklabel(0, gap / 2)
    femm.ei_selectlabel(0, gap / 2)
    femm.ei_setblockprop("FR4", 1, 0, 0)
    femm.ei_clearselected()

    femm.ei_makeABC(7, 60, 0, gap / 2, 0)
    femm.ei_addblocklabel(0, 40)
    femm.ei_selectlabel(0, 40)
    femm.ei_setblockprop("Air", 1, 0, 0)
    femm.ei_clearselected()

    femm.ei_saveas("capacitor.fee")
    femm.ei_analyze(1)
    femm.ei_loadsolution()
    v, q = femm.eo_getconductorproperties("plus")
    c = q / v                              # F per metre of depth
    print("gap = %.1f mm   C = %.2f pF/m" % (gap, c * 1e12))
    rows.append((gap, c * 1e12))
    femm.eo_close()
    femm.ei_close()

with open("capacitor_sweep.csv", "w") as f:
    f.write("gap_mm,C_pF_per_m\n")
    for g, c in rows:
        f.write("%.3f,%.4f\n" % (g, c))
print("Wrote capacitor_sweep.csv")
