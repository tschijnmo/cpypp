import os.path
import subprocess

def FlagsForFile(filename, **kwargs):
    flags = ['-std=c++14', '-I/usr/local/include', '-I.']

    # Call the config script to get the directories for the Python3 interpreter
    # in PATH.
    py_includes = subprocess.check_output(
        ['python3-config', '--includes']
    ).decode('utf-8').split()
    flags.extend(py_includes)

    proj_dir = os.path.dirname(os.path.realpath(__file__))
    proj_include = os.path.join(proj_dir, 'include')
    flags.append('-I' + proj_include)

    return {'flags': flags}
