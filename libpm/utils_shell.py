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


class UtilsShellOutput:
    """ helper UtilsShellOutput """
    def __init__(self, _log=None):
        self.__log = _log
        self.__collect_output = []

    def debug(self, _msg):
        """ debug """
        self.__collect_output += [_msg]
        if self.__log is not None:
            self.__log.debug(_msg)

    def error(self, _msg):
        """ error """
        print(_msg)
        self.__collect_output += [_msg]
        if self.__log is not None:
            self.__log.error(_msg)

    def get_output(self):
        """ return output """
        return "\n".join(self.__collect_output)


class UtilsShell:
    """ UtilsShell """

    @staticmethod
    def run_command(_command, _log=None, exit_on_error=True, print_output_on_exit=False, env=None):
        """ run_command """
        output = UtilsShellOutput(_log)
        output.debug("Run command: " + " ".join(_command))
        with subprocess.Popen(
            _command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=False,
            env=env
        ) as _proc:
            _returncode = _proc.poll()
            while _returncode is None:
                _returncode = _proc.poll()
                _line = _proc.stdout.readline()
                if _line:
                    _line = _line.decode("utf-8").rstrip()
                    output.debug(_line)
            while _line:
                _line = _proc.stdout.readline()
                if _line:
                    _line = _line.decode("utf-8").rstrip()
                    output.debug(_line)
                else:
                    break
            if _returncode != 0:
                _msg = "ERROR: returncode " + str(_returncode) + "\n"
                output.error(_msg)
                if exit_on_error:
                    if print_output_on_exit:
                        print(output.get_output())
                    sys.exit(_returncode)
            return _returncode, output.get_output()
        output.error("ERROR: Could not start process")
        if exit_on_error:
            if print_output_on_exit:
                print(output.get_output())
            sys.exit(-1)
        return -1, output.get_output()

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
