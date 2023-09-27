import shutil
import os
import json

# path to source directory
src_dir = 'template\generator_template'
# path to destination directory
output_name = "firmware_1"
dest_dir = 'result\{}'.format(output_name)

# getting all the files in the source directory
files = os.listdir(src_dir)
shutil.copytree(src_dir, dest_dir)

# Editing env data
data = json.load(open(dest_dir + '\spiffs_data\data.json', "r"))
data["name"] = "Pawan"
data["age"] = 21
json.dump(data, open(dest_dir + '\spiffs_data\data.json', "w"))

# Editing Project file
with open("{}\CMakeLists.txt".format(dest_dir), "r") as cmake:
    txt = cmake.read()
    txt = txt.replace("generator_template", output_name)
    
with open("{}\CMakeLists.txt".format(dest_dir), "w") as cmake:
    cmake.write(txt)
