# preBuild.py
Import('env')
from shutil import copyfile
import subprocess
import inspect, os.path

 
from preBuildHTML import preBuildHTMLFun
from preBuildConfig import preBuildConfigFun
from preBuildDash import preBuildDashFun

filename = inspect.getframeinfo(inspect.currentframe()).filename
dir_path = os.path.dirname(os.path.abspath(filename))

# default setting = rebuild config and do not rebuild HTML or certificates
html = False
config = False
dash = False

# private library flags
for item in env.get("CPPDEFINES", []):
    print("preBuild.py item:",item)
    if item == "REBUILD_HTML":
        html = True
        config = True
        dash = True
    elif item == "REBUILD_CONFIG":
        config = True
    elif item == "REBUILD_DASHBOARD":
        dash = True

if html:
    preBuildHTMLFun()
if config:
    preBuildConfigFun(env)
if dash:
    preBuildDashFun()
