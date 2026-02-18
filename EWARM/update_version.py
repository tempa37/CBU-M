import re
import sys
import os
from datetime import datetime
import subprocess

os.chdir('.././Core/Inc')
file_name = sys.argv[1]
f = open(file_name, "r+")
v = f.read()

pattern_version_major = re.compile(r'VERSION_MAJOR (?P<major>\d*)')
match_major = re.search(pattern_version_major, v)
major_num = int(match_major.group('major'))

pattern_version_minor = re.compile(r'VERSION_MINOR (?P<minor>\d*)')
match_minor = re.search(pattern_version_minor, v)
minor_num = int(match_minor.group('minor'))

pattern_version_build = re.compile(r'VERSION_BUILD (?P<build>\d*)')
match_build  = re.search(pattern_version_build, v)
build_num = int(match_build.group('build'))

build_num+=1

if build_num >= 255:
	minor_num+=1
	build_num=0

if minor_num >= 255:
	major_num+=1
	minor_num=0	

new_version_major ="VERSION_MAJOR {0}".format(major_num)
v = re.sub(pattern_version_major, new_version_major, v)

new_version_minor ="VERSION_MINOR {0}".format(minor_num)
v = re.sub(pattern_version_minor, new_version_minor, v)

new_version_build = "VERSION_BUILD {0}".format(build_num)
v = re.sub(pattern_version_build, new_version_build, v)

out = "{0}.{1}.{2}".format(major_num, minor_num, build_num)

print(out)

now = datetime.now()

date_time = now.strftime("%m/%d/%Y, %H:%M:%S")

pattern_dt = re.compile(r'(?P<DT>\d{2}\/\d{2}\/\d{4}, \d{2}:\d{2}:\d{2})')

v = re.sub(pattern_dt, date_time, v)

f.seek(0)
f.truncate()
f.write(v)
f.close()

os.chdir('../.././Middlewares/Third_Party/LwIP/src/apps/http')

subprocess.run('makefsdata_2_2_0.cmd', check=True, shell=True)