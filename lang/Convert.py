#! /usr/bin/env python3

import sys
from collections import OrderedDict
import re

compiled_re = re.compile(r'\["(.*)"\]\s*=\s*"(.*)",?', re.UNICODE)

translated = OrderedDict()

with open(sys.argv[1], encoding="utf-8") as f:
    for l in f:
        l = l.strip()
        if len(l) == 0:
            continue
        if l.startswith("return") or l.startswith("{") or l.startswith("}") or l.startswith("--"):
            continue
        else:
            mch = compiled_re.match(l)
            if mch:
                translated[mch.group(1)] = mch.group(2)
            else:
                print("Unhandled line:", l)
                print("In file:", sys.argv[1])


import json

with open(sys.argv[1].replace(".lua", ".json"), "w", encoding="utf-8") as g:
    g.write(json.dumps(translated, indent=2, ensure_ascii=False))


