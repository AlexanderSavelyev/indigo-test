import os
import shutil
import sys
from os.path import *

from optparse import OptionParser

presets = {
    "win32" : ("Visual Studio 10", ""),
    "win64" : ("Visual Studio 10 Win64", ""),
    "linux32" : ("Unix Makefiles", "-DSUBSYSTEM_NAME=x86"),
    "linux64" : ("Unix Makefiles", "-DSUBSYSTEM_NAME=x64"),
    "mac10.5" : ("Xcode", ""),
}

parser = OptionParser(description='Indigo libraries build script')
parser.add_option('--generator', help='this option is passed as -G option for cmake')
parser.add_option('--params', default="", help='additional build parameters')
parser.add_option('--config', default="Release", help='project configuration')
parser.add_option('--no-build', default=False, 
    action="store_true", help='configure without building', dest="nobuild")
parser.add_option('--clean', default=False, action="store_true", 
    help='delete all the build data', dest="clean")
parser.add_option('--preset', type="choice", dest="preset", 
    choices=presets.keys(), help='build preset %s' % (str(presets.keys())))

(args, left_args) = parser.parse_args()
if len(left_args) > 0:
    print("Unexpected arguments: %s" % (str(left_args)))
    exit()

if args.preset:
    args.generator, args.params = presets[args.preset]
if not args.generator:
    print("Generator must be specified")
    exit()

cur_dir = abspath(dirname(__file__))
root = join(cur_dir, "..")
project_dir = join(cur_dir, "indigo-all")

build_dir = (args.generator + " " + args.params)
build_dir = build_dir.replace(" ", "_").replace("=", "_").replace("-", "_")

full_build_dir = os.path.join(root, "build", build_dir)
if os.path.exists(full_build_dir) and args.clean:
    print("Removing previous project files")
    shutil.rmtree(full_build_dir)
if not os.path.exists(full_build_dir):
    os.makedirs(full_build_dir)

os.chdir(full_build_dir)
os.system("cmake -G \"%s\" %s %s" % (args.generator, args.params, project_dir))

if args.nobuild:
    exit(0)

for f in os.listdir(full_build_dir):
    path, ext = os.path.splitext(f)
    if ext == ".zip":
        os.remove(join(full_build_dir, f))

os.system("cmake --build . --config %s" % (args.config))
if args.generator.find("Unix Makefiles") != -1:
    os.system("make package")
    os.system("make install")
else if args.generator.find("Xcode") != -1:
    os.system("cmake --build . --target package --config %s" % (args.config))
    os.system("cmake --build . --target install --config %s" % (args.config))
else if args.generator.find("Visual Studio") != -1:
    os.system("cmake --build . --target PACKAGE --config %s" % (args.config))
    os.system("cmake --build . --target INSTALL --config %s" % (args.config))
else:
    print("Do not know how to run package and install target")

os.chdir(root)
if not os.path.exists("dist"):
    os.mkdir("dist")
dist_dir = join(root, "dist")
    
for f in os.listdir(full_build_dir):
    path, ext = os.path.splitext(f)
    if ext == ".zip":
        shutil.copy(join(full_build_dir, f), join(dist_dir, f))
