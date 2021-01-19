#!/usr/bin/python3

import sys
import re
from pathlib import Path
import os

cfiles = []
hfiles = []

d = Path(sys.argv[1]).resolve()
for path in d.rglob('*.c'):
    cfiles += [path]
for path in d.rglob('*.h'):
    hfiles += [path]

print(cfiles)
print(hfiles)

for fn in cfiles + hfiles:
	print(fn)
	pd = fn.parent
	lines = open(fn).readlines()
	f = open(fn, "w")
	for line in lines:
		m = re.match('# *include *("|<)([^">]*)("|>)', line)
		if m:
			hname = Path(m.group(2)).name
			found = False
			for hf in hfiles:
				if hname == hf.name:
					assert not found
					found = True
					print("",os.path.relpath(hf, pd))
					f.write('#include "' + os.path.relpath(hf, pd) + '"\n')
			if not found:
				f.write(line)
		else:
			f.write(line)
