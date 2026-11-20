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

""" Example logger for commands """

import logging
import sys


class UtilsLog:
    """ UtilsLog """
    def __init__(self, name, filepath=None):
        self.__log = logging.getLogger(name)
        formatter = logging.Formatter(
            '[%(asctime)s] %(levelname)s [%(filename)s.%(funcName)s:%(lineno)d] %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )

        # INFO / DEBUG -> stdout  (PowerShell won't wrap these)
        self.__stdout_handler = logging.StreamHandler(sys.stdout)
        self.__stdout_handler.setLevel(logging.INFO)
        self.__stdout_handler.addFilter(lambda r: r.levelno < logging.WARNING)
        formatter = logging.Formatter("[%(levelname)s] %(name)s: %(message)s")
        self.__stdout_handler.setFormatter(formatter)

        # WARNING / ERROR / CRITICAL -> stderr
        self.__stderr_handler = logging.StreamHandler(sys.stderr)
        self.__stderr_handler.setLevel(logging.WARNING)
        self.__stderr_handler.setFormatter(formatter)

        self.__log.setLevel(logging.DEBUG)
        self.__log.propagate = False
        self.__log.addHandler(self.__stdout_handler)
        self.__log.addHandler(self.__stderr_handler)

        if filepath is not None:
            self.__file_handler = logging.FileHandler(filepath)
            self.__file_handler.setLevel(logging.DEBUG)
            self.__log.addHandler(self.__file_handler)

    def get_logger(self):
        """ return configured logger """
        return self.__log

    def set_level(self, level):
        """ set level """
        self.__log.setLevel(level)
        self.__stdout.setLevel(level)
