# Utility functions for the broad testing script

import os.path
import sys
import subprocess
import h5py

SOFTPATH = '../../build/src/soft'

def error(msg):
    print('ERROR: '+msg)
    sys.exit(1)

def init():
    """
    Initialize this module.
    """
    global SOFTPATH

    if not os.path.isfile(SOFTPATH):
        SOFTPATH = '../'+SOFTPATH
        if not os.path.isfile(SOFTPATH):
            SOFTPATH = '../'+SOFTPATH
            if not os.path.isfile(SOFTPATH):
                raise RuntimeError('Unable to find SOFT executable.')
    
def runSOFT(pi):
    """
    Run SOFT, passing on 'pi' on stdin to the executable.
    """
    global SOFTPATH

    p = subprocess.Popen([SOFTPATH], stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)

    stderr_data = p.communicate(input=bytearray(pi, 'ascii'))[1].decode('utf-8')

    if p.returncode is not 0:
        raise RuntimeError("SOFT exited with a non-zero exit code ("+str(p.returncode)+").\n"+stderr_data)

    return stderr_data

class Image:
    def __init__(self, filename):
        with h5py.File(filename, 'r') as f:
            self.image = f['image'][:,:]
            self.detectorPosition = f['detectorPosition'][:,:]
            self.detectorDirection = f['detectorDirection'][:,:]
            self.detectorVisang = f['detectorVisang'][:,:]
            self.wall = f['wall'][:,:]

class Green:
    def __init__(self, filename):
        with h5py.File(filename, 'r') as f:
            self.func = f['func'][:]
            self.param1 = f['param1'][:]
            self.param2 = f['param2'][:]
            self.r      = f['r'][:]
            self.wavelengths = f['wavelengths'][:]

            self.param1name = self.tostring(f['param1name'])
            self.param2name = self.tostring(f['param2name'])
            self.format     = self.tostring(f['type'])

    def tostring(self, arr):
        """
        Convert the given MATLAB string to a Python string.
        """
        return "".join(map(chr, arr[:,:][:,0].tolist()))

class Spectrum:
    def __init__(self, filename):
        with h5py.File(filename, 'r') as f:
            self.I = f['I'][:,:]
            self.wavelengths = f['wavelengths'][:,:]

            if 'Q' in f: self.Q = f['Q'][:,:]
            else: self.Q = None

            if 'U' in f: self.U = f['U'][:,:]
            else: self.U = None

            if 'V' in f: self.V = f['V'][:,:]
            else: self.V = None
            
