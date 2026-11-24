#!/usr/bin/env python3
##################################################################################
#
#            ░░░░░░░
#           ░██▓▓▓▓▒
#  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
#  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
#   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
#  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
#   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
#
# MIT License
#
# Copyright (c) 2026 Evgenii Sopov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Original repository: https://github.com/sea5kg/s5run
#
##################################################################################

"""
Helper tool for smoke tests
"""

import os
import shutil

from .pm_config import FolderSwitcher
from .utils_shell import UtilsShell

ROOT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.join(ROOT_DIR, "..")
ROOT_DIR = os.path.normpath(ROOT_DIR)


class UtilsSmokeTests:
    """ UtilsSmokeTests """

    @staticmethod
    def compile_s5run(_log):
        """ compile_s5run """
        with FolderSwitcher(_log, ROOT_DIR) as _:
            UtilsShell.run_command(
                ["./build_simple.sh"],
                _log=_log,
                print_output_on_exit=True
            )

    @staticmethod
    def remove_if_dir_exists(_log, _parent_dir, _sub_dir):
        """ remove_if_dir_exists """
        _dir = os.path.join(_parent_dir, _sub_dir)
        if os.path.isdir(_dir):
            shutil.rmtree(_dir)
            _log.debug("Removed: %s", _dir)
            return 1
        return 0

    @staticmethod
    def clean_generated_outputs(_log):
        """ Delete all generated code AND build artifacts to guarantee a fresh build. """
        _tests_dir = os.path.join(ROOT_DIR, "tests")
        _tests_dir = os.path.normpath(_tests_dir)
        _counter = 0
        for _dir in os.listdir(_tests_dir):
            _dir = os.path.join(_tests_dir, _dir)
            if not os.path.isdir(_dir):
                continue
            # print(_dir)
            _counter += UtilsSmokeTests.remove_if_dir_exists(_log, _dir, "s5run_cli")  # cpp
            _counter += UtilsSmokeTests.remove_if_dir_exists(_log, _dir, "s5run_cli_example")  # cpp
            _counter += UtilsSmokeTests.remove_if_dir_exists(_log, _dir, "s5runcli")  # go
            _counter += UtilsSmokeTests.remove_if_dir_exists(_log, _dir, "src/com")  # java
            _counter += UtilsSmokeTests.remove_if_dir_exists(_log, _dir, "bin")  # go, cpp

            # *.jar files
            for _file in os.listdir(_dir):
                _file = os.path.join(_dir, _file)
                if _file.endswith(".jar"):
                    os.remove(_file)
                    _counter += 1
                    _log.debug("Removed: %s", _file)
        if _counter > 0:
            _log.info("-> Cleanup. " + str(_counter) + " files and dirs cleaned.")
        else:
            _log.info("-> Cleanup. Nothing to clean")
        # sys.exit(1)

    @staticmethod
    def clean_binaries(_log):
        """ Delete all generated code AND build artifacts to guarantee a fresh build. """
        _tests_dir = os.path.join(ROOT_DIR, "tests")
        _tests_dir = os.path.normpath(_tests_dir)
        _counter = 0
        for _dir in os.listdir(_tests_dir):
            _dir = os.path.join(_tests_dir, _dir)
            if not os.path.isdir(_dir):
                continue
            _counter += UtilsSmokeTests.remove_if_dir_exists(_log, _dir, "bin")  # go, cpp

            # *.jar files
            for _file in os.listdir(_dir):
                _file = os.path.join(_dir, _file)
                if _file.endswith(".jar"):
                    os.remove(_file)
                    _counter += 1
                    _log.debug("Removed: %s", _file)
        if _counter > 0:
            _log.info("-> Cleanup binaries. " + str(_counter) + " files and dirs cleaned.")
        else:
            _log.info("-> Cleanup binaries. Nothing to clean")
        # sys.exit(1)
