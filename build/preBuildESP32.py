#!/usr/bin/env python3

Import('env')
from preBuildExampleFile import preBuildExampleFile
from preBuildESP32Certificates import preBuildCertificates

preBuildExampleFile()
preBuildCertificates(env)