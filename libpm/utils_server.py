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

import os
import time
import socket
import subprocess

ROOT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.join(ROOT_DIR, "..")
ROOT_DIR = os.path.normpath(ROOT_DIR)


class UtilsServer:
    """ UtilsServer """

    def __init__(self, _log, command):
        self.__log = _log
        self.__port = UtilsServer.free_port()
        self.__proc = None
        self.__command = []
        for _cmd in command:
            if "$(FREE_PORT)" in _cmd:
                _cmd = _cmd.replace("$(FREE_PORT)", str(self.__port))
            self.__command.append(_cmd)
        # print(self.__command[0])
        self.__basename = os.path.basename(self.__command[0])
        self.__server_log = open(os.path.join(ROOT_DIR, "tmp", self.__basename + "-server.log"), "w")
        self.__dir = os.path.dirname(os.path.realpath(self.__command[0]))

    @staticmethod
    def free_port():
        """ find free port """
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(("127.0.0.1", 0))
        port = s.getsockname()[1]
        s.close()
        return port

    def port(self):
        """ return port """
        return self.__port

    def start(self):
        """ start """
        self.__log.info("-> Start server %s on %d port... ", self.__basename, self.__port)
        self.__proc = subprocess.Popen(
            self.__command,
            cwd=self.__dir,
            stdout=self.__server_log,
            stderr=subprocess.STDOUT
        )
        deadline = time.time() + 10
        while time.time() < deadline:
            try:
                s = socket.create_connection(("127.0.0.1", self.__port), timeout=0.5)
                s.close()
                return self.__proc
            except OSError:
                if self.__proc.poll() is not None:
                    raise SystemExit("!!! FAIL: server exited with code %d" % self.__proc.returncode)
                time.sleep(0.05)
        self.__proc.terminate()
        raise SystemExit("!!! FAIL: server did not start in 10s")

    def stop(self):
        """ stop """
        if self.__proc is not None:
            self.__proc.terminate()
            try:
                self.__proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.__proc.kill()
                self.__proc.wait()
