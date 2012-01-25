import os
import shutil
import sys
from os.path import *

import argparse
parser = argparse.ArgumentParser(description='Indigo libraries build script', 
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('generator', help='this option is passed as -G option for cmake')
parser.add_argument('-params', '-p', default="", help='additional build parameters')
parser.add_argument('-config', '-c', default="Release", help='project configuration')

args = parser.parse_args()

cur_dir = abspath(dirname(__file__))
root = join(cur_dir, "..")
project_dir = join(cur_dir, "indigo-all")

build_dir = (args.generator + " " + args.params).replace(" ", "_")

full_build_dir = os.path.join(root, "build", build_dir)
if not os.path.exists(full_build_dir):
    os.makedirs(full_build_dir)

os.chdir(full_build_dir)
os.system("cmake -G \"%s\" %s %s" % (args.generator, args.params, project_dir))

for f in os.listdir(full_build_dir):
    path, ext = os.path.splitext(f)
    if ext == ".zip":
        os.remove(join(full_build_dir, f))

os.system("cmake --build . --config %s" % (args.config))
if args.params.find("Makefiles") != -1:
    os.system("make package")
    os.system("make install")
else:
    os.system("cmake --build . --target PACKAGE --config %s" % (args.config))
    os.system("cmake --build . --target INSTALL --config %s" % (args.config))

os.chdir(root)
if not os.path.exists("dist"):
    os.mkdir("dist")
dist_dir = join(root, "dist")
    
for f in os.listdir(full_build_dir):
    path, ext = os.path.splitext(f)
    if ext == ".zip":
        shutil.copy(join(full_build_dir, f), join(dist_dir, f))
