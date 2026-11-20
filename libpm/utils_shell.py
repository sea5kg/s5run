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

""" Helpers for run shell """

import os
import sys
import subprocess


class UtilsShell:
    """ UtilsShell """

    @staticmethod
    def run_command(_command, _log=None, exit_on_error=True, print_output_on_exit=False):
        """ run_command """
        _collect_output = []
        _msg = "Run command: " + " ".join(_command) + "\n"
        _collect_output += [_msg]
        if _log is not None:
            _log.debug(_msg)
        with subprocess.Popen(
            _command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=False
        ) as _proc:
            _returncode = _proc.poll()
            while _returncode is None:
                _returncode = _proc.poll()
                _line = _proc.stdout.readline()
                if _line:
                    _line = _line.decode("utf-8").strip()
                    _collect_output += [_line]
                    if _log is not None:
                        _log.debug(_line)
            while _line:
                _line = _proc.stdout.readline()
                if _line:
                    _line = _line.decode("utf-8").strip()
                    _collect_output += [_line]
                    if _log is not None:
                        _log.debug(_line + "\n")
                else:
                    break
            if _returncode != 0:
                print("ERROR: returncode " + str(_returncode))
                _msg = "ERROR: returncode " + str(_returncode) + "\n"
                _collect_output += [_msg]
                if _log is not None:
                    _log.error(_msg)
                if exit_on_error:
                    if print_output_on_exit:
                        print("\n".join(_collect_output))
                    sys.exit(_returncode)
            return _returncode
        _msg = "ERROR: Could not start process"
        _collect_output += [_msg]
        _log.error(_msg)
        if exit_on_error:
            if print_output_on_exit:
                print("\n".join(_collect_output))
            sys.exit(-1)
        return -1

    @staticmethod
    def run_command_get_output(_log, _command):
        """ run_command_get_output """
        # _log.info("Run command: " + " ".join(_command))
        _output = ""
        with subprocess.Popen(
            _command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=False
        ) as _proc:
            _returncode = _proc.poll()
            while _returncode is None:
                _returncode = _proc.poll()
                _line = _proc.stdout.readline()
                if _line:
                    _line = _line.decode("utf-8").strip()
                    _output += _line + "\n"
            while _line:
                _line = _proc.stdout.readline()
                if _line:
                    _line = _line.decode("utf-8").strip()
                    _output += _line + "\n"
                else:
                    break
            if _returncode != 0:
                # _log.error("ERROR: returncode %s", str(_returncode))
                return _returncode, _output
        return _returncode, _output

    @staticmethod
    def current_script_path():
        """ current_script_path """
        _dir = os.path.dirname(__file__)
        _dir = os.path.join(_dir, "..")
        _dir = os.path.normpath(_dir)
        return _dir
