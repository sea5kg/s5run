#!/usr/bin/env python3

import os

from libs5runcligen import S5RunCliSchema, Generator

ROOT_DIR = os.path.dirname(os.path.realpath(__file__))

schema = S5RunCliSchema()
schema.load(os.path.join(ROOT_DIR, "s5run-client.json"))

gen = Generator(schema)
gen.generate_all()
print(gen.report())
