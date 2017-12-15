import subprocess

def FlagsForFile(filename, **kwargs):
    flags = ['-std=c++14', '-I/usr/local/include']

    # Call the config script to get the directories for the Python3 interpreter
    # in PATH.
    py_includes = subprocess.check_output(
        ['python3-config', '--includes']
    ).decode('utf-8').split()
    flags.extend(py_includes)

    return {'flags': flags}
