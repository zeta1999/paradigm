from argparse import ArgumentParser
import os 
import sys
import platform
import shutil
import subprocess
from . import patch

import functools
print = functools.partial(print, flush=True)

def onerror(func, path, exc_info):
    """
    Error handler for ``shutil.rmtree``.

    If the error is due to an access error (read only file)
    it attempts to add write permission and then retries.

    If the error is for another reason it re-raises the error.
    
    Usage : ``shutil.rmtree(path, onerror=onerror)``
    """
    import stat
    if not os.access(path, os.W_OK):
        # Is the error an access error ?
        os.chmod(path, stat.S_IWUSR)
        func(path)
    else:
        print(path)
        raise

class bcolors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    NC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
        
class Tester(object):        
    def initialize(self, parser = []):
        args = ArgumentParser(description='Generate build files for the current project.')
        args.add_argument("--platform", "-t", "--target", action='store', type=str, nargs=None,choices=['ubuntu','windows'],dest="platform")
        if not parser:
            return args.parse_known_args()
        else:
            return args.parse_known_args(parser)
        
    def test(self, args, remaining, directory = os.path.dirname(os.path.realpath(__file__))+"/../"):
        if args.platform == "ubuntu":
            image = "ubuntu-clang"
        else:
            image = "windows-vcpp"
        print("booting up docker image '", image, "'...")
        print("\t with arguments: ", *remaining)
        if subprocess.check_call(["docker", "run", "--rm", "--mount", "type=bind,source="+directory+",destination=/paradigm_local_git/,readonly", "--name=temp", image, "--remote", "file:////paradigm_local_git", "--build"] + remaining, shell=False) != 0:
            return
    
    def __call__(self, parser = []):
        args, remaining = self.initialize(parser)
        self.test(args, remaining)