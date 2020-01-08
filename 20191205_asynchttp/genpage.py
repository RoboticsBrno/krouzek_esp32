#!/usr/bin/env python

import json

with open("src/index.html", "rb") as infile, open("src/webpage.h", "w") as outfile:
    instring = infile.read().decode("utf-8")
    outstring = json.dumps(instring).strip('"').replace("\r", "")
    outfile.write("#pragma once\n\n")
    outfile.write("const char index_html[] PROGMEM =\"")
    outfile.write(outstring)
    outfile.write("\";\n")
#string_for_printing = json.dumps(original_string).strip('"')
