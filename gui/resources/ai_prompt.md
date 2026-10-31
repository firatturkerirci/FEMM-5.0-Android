You are writing a Python script for FEMM 5 (femm-qt), a 2D finite element
program for magnetics, electrostatics, heat flow and current flow. I will paste
your script into FEMM and run it there. Follow these rules exactly.

ENVIRONMENT
- The script runs inside FEMM's embedded MicroPython, not CPython.
- Use the pyFEMM API: `import femm`, then `femm.openfemm()`, `femm.newdocument(n)`
  and the usual `femm.mi_*`, `femm.mo_*`, `femm.ei_*`, `femm.eo_*`, `femm.hi_*`,
  `femm.ho_*`, `femm.ci_*`, `femm.co_*` functions, with the same arguments as in
  the FEMM 4.2 manual. newdocument: 0 magnetics, 1 electrostatics, 2 heat flow,
  3 current flow.
- Available: math, `import numpy as np` (a numpy SUBSET: array, zeros, ones,
  linspace, arange, sin, cos, sqrt, exp, sum, max, min, mean, dot, linalg.inv),
  json, re, time, random, csv, os.
- NOT available: matplotlib, scipy (beyond tiny parts), pandas, sympy, and any
  package that needs pip. Do not import them. No plotting: report numbers with
  print() and write tables to a .csv file with open()/csv.
- No input(), no GUI dialogs, no network.

FILES
- Use relative file names only (for example "model.fem", "results.csv"). They
  are created in FEMM's work folder.
- Always save before solving: `femm.mi_saveas("name.fem")` (ei_/hi_/ci_ for the
  other physics), then `femm.mi_analyze(1)` and `femm.mi_loadsolution()`.

MODEL BUILDING
- Call mi_probdef first: `femm.mi_probdef(freq, "millimeters", "planar" or "axi",
  1e-8, depth, 30)`. Axisymmetric models must lie at r >= 0.
- Draw closed regions with mi_drawrectangle / mi_drawline / mi_drawarc. Every
  closed region needs exactly one block label:
  mi_addblocklabel(x, y); mi_selectlabel(x, y);
  mi_setblockprop(material, 1, 0, circuit_or_"<None>", magdir, group, turns);
  mi_clearselected().
- Select by coordinates of a point ON the object; always mi_clearselected() after.
- Use groups (the group argument) for regions you later integrate over, then
  `mo_groupselectblock(group)` and `mo_blockintegral(type)`.
- Open boundary: `femm.mi_makeABC(7, radius, x_center, y_center, 0)` after the
  geometry is drawn, with radius about 3-5 times the model size.
- Circuits: `mi_addcircprop(name, current_A, 1)` (1 = series).

MATERIALS
- Library names must match EXACTLY, including spaces and brackets. Safe
  magnetics names: "Air", "Copper", "Aluminum, 1100", "Pure iron, annealed",
  "1010 Steel (hot rolled)", "1018 Steel (hot rolled)", "1020 Steel (hot rolled)",
  "M-19 14 mil", "M-27 14 mil", "M-36 14 mil", "M-43 18 mil",
  "304 Stainless Steel", "316 Stainless Steel", "N35", "N42", "N52",
  "Ceramic 5", "Ceramic 8A", "SmCo24", "Cast Alnico 5 (LNG40)",
  wire gauges "10 AWG", "12 AWG", ... "36 AWG" (even numbers only),
  "0.5mm", "1mm".
  Get each with `femm.mi_getmaterial(name)` before using it.
- For anything else, define the material yourself with mi_addmaterial (or
  ei_addmaterial / hi_addmaterial / ci_addmaterial) and give it a clear name.
  For a nonlinear steel not in the list above, add a B-H curve with
  mi_addbhpoint.

OUTPUT
- Print every result with units, for example
  `print("Inductance L = %.4f mH" % (L * 1e3))`.
- For sweeps, loop over the parameter, solve each case, print one line per case
  and also write all rows to a .csv file.
- Keep the whole model in ONE self-contained script.

WHEN I SEND YOU AN ERROR
- FEMM reports errors as "Line N: message" with the surrounding lines. Fix that
  problem, keep the rest of the script unchanged, and send the complete script.

EXAMPLE (axisymmetric solenoid with a steel plunger)

```python
import femm
femm.openfemm()
femm.newdocument(0)
femm.mi_probdef(0, "millimeters", "axi", 1e-8, 0, 30)
femm.mi_getmaterial("Air")
femm.mi_getmaterial("1010 Steel (hot rolled)")
femm.mi_getmaterial("18 AWG")
femm.mi_addcircprop("coil", 1.0, 1)
femm.mi_drawrectangle(6, -20, 16, 20)     # coil
femm.mi_drawrectangle(0, -10, 5, 30)      # plunger
femm.mi_addblocklabel(11, 0); femm.mi_selectlabel(11, 0)
femm.mi_setblockprop("18 AWG", 1, 0, "coil", 0, 1, 500); femm.mi_clearselected()
femm.mi_addblocklabel(2.5, 10); femm.mi_selectlabel(2.5, 10)
femm.mi_setblockprop("1010 Steel (hot rolled)", 1, 0, "<None>", 0, 2, 0); femm.mi_clearselected()
femm.mi_addblocklabel(30, 30); femm.mi_selectlabel(30, 30)
femm.mi_setblockprop("Air", 1, 0, "<None>", 0, 0, 0); femm.mi_clearselected()
femm.mi_makeABC(7, 100, 0, 0, 0)
femm.mi_saveas("solenoid.fem")
femm.mi_analyze(1)
femm.mi_loadsolution()
i, v, flux = femm.mo_getcircuitproperties("coil")
femm.mo_groupselectblock(2)
fz = femm.mo_blockintegral(19)
femm.mo_clearblock()
print("Inductance L = %.4f mH" % (flux / i * 1e3))
print("Plunger force Fz = %.4f N" % fz)
```

Now write the script for this model:
