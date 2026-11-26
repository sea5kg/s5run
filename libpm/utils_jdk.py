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
Helper tool for find and detect java - jdk
"""

import os
import shutil
import subprocess

from .pm_config import FolderSwitcher
from .utils_shell import UtilsShell

ROOT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.join(ROOT_DIR, "..")
ROOT_DIR = os.path.normpath(ROOT_DIR)


class UtilsJDK:
    """ UtilsSmokeTests """

    @staticmethod
    def have_jdk():
        """ check have java """
        return all(shutil.which(cmd) for cmd in ("javac", "jar", "java"))

    @staticmethod
    def jdk_path(version):
        """JAVA_HOME override for a specific Java version, e.g. _jdk_path(11)."""
        env = os.environ.get(f"JAVA{version}_HOME", "")
        if env and os.path.isfile(os.path.join(env, "bin", "javac")):
            return env
        candidates = [
            f"/usr/lib/jvm/java-{version}-openjdk-amd64",
            f"/usr/lib/jvm/java-{version}-openjdk",
            f"/usr/lib/jvm/java-{version}-openjdk-arm64",
        ]
        for c in candidates:
            if os.path.isfile(os.path.join(c, "bin", "javac")):
                return c
        return None

    @staticmethod
    def have_jdk_version(version):
        """True if this Java version target is buildable: a matching JDK is
        installed OR the default javac is new enough to cross-compile via
        --release."""
        if UtilsJDK.jdk_path(version):
            return True
        if UtilsJDK.have_jdk():
            try:
                out = subprocess.check_output(["javac", "--version"], stderr=subprocess.STDOUT, text=True)
                major = int(out.strip().split()[1].split(".")[0])
                return major >= version
            except (ValueError, subprocess.CalledProcessError, FileNotFoundError):
                return False
        return False

    @staticmethod
    def java_build_env(version):
        """Env with JAVA_HOME and JAVA_RELEASE for the target version.

        Uses the system javac with --release N when no dedicated JDK<N> is
        installed (cross-compilation)."""
        env = os.environ.copy()
        jdk = UtilsJDK.jdk_path(version)
        if jdk:
            env["JAVA_HOME"] = jdk
            env["PATH"] = os.path.join(jdk, "bin") + os.pathsep + env["PATH"]
            env["JAVA_RELEASE"] = str(version)
        elif UtilsJDK.have_jdk():
            # cross-compile through the default (newer) javac
            env["JAVA_RELEASE"] = str(version)
        return env