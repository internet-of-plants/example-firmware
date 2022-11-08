#!/usr/bin/env python3

Import('env')

from os import path
import inspect
import random
import string

filename = inspect.getframeinfo(inspect.currentframe()).filename
destination = path.join(path.dirname(path.abspath(filename)), "../include/generated/", "psk.hpp")

if not path.exists(destination):
    print("Generating " + destination)

    with open(destination, "w", encoding="utf8") as f:
        f.write("#ifndef IOP_PSK_H\n")
        f.write("#define IOP_PSK_H\n\n")

        f.write("// This file is computer generated at build time (`build/preBuild.py` called by PlatformIO)\n\n")\

        f.write("#include <iop-hal/string.hpp>\n\n")

        f.write("namespace generated {\n")
        f.write("constexpr static char PSK[] IOP_ROM = \"")
        f.write(''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for _ in range(63)))
        f.write("\";\n")
        f.write("} // namespace generated\n")
        f.write("#endif")