from subprocess import call
import subprocess
from win32api import GetFileVersionInfo, LOWORD, HIWORD
import zipfile
import os

def get_version_number (filename):
  info = GetFileVersionInfo (filename, "\\")
  ms = info['FileVersionMS']
  ls = info['FileVersionLS']
  return '{}.{}.{}.{}'.format (HIWORD (ms), LOWORD (ms), HIWORD (ls), LOWORD (ls))

def zip(src, dst):
	zf = zipfile.ZipFile(dst, "w", zipfile.ZIP_DEFLATED)
	zf.write(src, os.path.basename(src))
	zf.close()

for arch in ['x64', 'x86']:
	dir = 'build-msvc2017-{}'.format (arch)
	build_args = ['cmake', '--build', dir, '--config', 'Release']
	FNULL = open(os.devnull, 'w')
	call(build_args, stdout=FNULL, stderr=subprocess.STDOUT)
	print ('Building {} version...'.format (arch))
	binary_path = os.path.join (dir, 'Release', 'DSpellCheck.dll')
	target_zip_path = os.path.join ('out', str (get_version_number (binary_path)),
		'DSpellCheck_{}.zip'.format (arch))
	target_zip_dir = os.path.dirname (target_zip_path)
	if not os.path.isdir (target_zip_dir):
		os.makedirs (target_zip_dir)
	print ('Deploying to {}...'.format (target_zip_path))
	zip (binary_path, target_zip_path)
successString = 'Success!'
try:
	from colorama import init, Fore, Style
	init ()
	successString = Fore.GREEN + Style.BRIGHT + successString
except:
	pass
print (successString)