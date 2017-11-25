from subprocess import call
import subprocess
from win32api import GetFileVersionInfo, LOWORD, HIWORD
import zipfile
import os
import argparse
import re
import sys
import shutil
from bs4 import BeautifulSoup

def get_version_number (filename):
	info = GetFileVersionInfo (filename, "\\")
	ms = info['FileVersionMS']
	ls = info['FileVersionLS']
	return '{}.{}.{}.{}'.format (HIWORD (ms), LOWORD (ms), HIWORD (ls), LOWORD (ls))

version_str = re.compile (r'(\s*VALUE "FileVersion", ")([0-9.]*)(")')

def get_rc_version ():
	with open('src/DSpellCheck.rc') as f:
		for line in f:
			m = version_str.match (line)
			if m:
				return m.group (2)

comma_version_str = [
re.compile (r'(\s*VALUE "ProductVersion", ")([0-9,]*)(")'),
re.compile (r'(\s*FILEVERSION\s*)([0-9,]*)()'),
re.compile (r'(\s*PRODUCTVERSION\s*)([0-9,]*)()')
]

def replace_rc_version (version):
	source_rc_path = 'src/DSpellCheck.rc'
	target_rc_path = 'src/DSpellCheck.rc.tmp'
	with open(target_rc_path, 'w') as fw:
		with open(source_rc_path) as f:
			for line in f:
				m = version_str.match (line)
				if m:
					line = re.sub (version_str, r'\g<1>{}\3'.format ('.'.join (version)), line)
				for pattern in comma_version_str:
					m = pattern.match (line)
					if m:
						line = re.sub (pattern, r'\g<1>{}\3'.format (','.join (version) + ',0'), line)
				fw.write (line)
	os.remove (source_rc_path)
	os.rename (target_rc_path, source_rc_path)

def zip(src, dst):
	zf = zipfile.ZipFile(dst, "w", zipfile.ZIP_DEFLATED)
	zf.write(src, os.path.basename(src))
	zf.close()

parser = argparse.ArgumentParser()
parser.add_argument('--new-minor', action="store_true", dest="new_minor", help="Deploy new minor version", default=False)
parser.add_argument('-v', '--verbose', action="store_true", dest="verbose", help="Verbose output", default=False)
options = parser.parse_args()
if options.new_minor:
	ver = get_rc_version ().split ('.')
	ver[-1]=str (int (ver[-1]) + 1)
	replace_rc_version (ver)
	print ('Version increased to {}'.format ('.'.join (ver)))

for arch in ['x64', 'x86']:
	'''
	dir = 'build-deploy-msvc2017-{}'.format (arch)
	build_args = ['cmake', '--build', dir, '--config', 'RelWithDebInfo']
	FNULL = open(os.devnull, 'w')
	print ('Building {} version...'.format (arch))
	binary_path = os.path.join (dir, 'RelWithDebInfo', 'DSpellCheck.dll')
	pdb_path = os.path.join (dir, 'RelWithDebInfo', 'DSpellCheck.pdb')
	if call(build_args, stdout= (None if options.verbose else FNULL)) != 0:
		print ('Error: Build error')
		sys.exit (1)
	out_path = os.path.join ('out', str (get_version_number (binary_path)))
	target_zip_path = os.path.join (out_path, 'DSpellCheck_{}.zip'.format (arch))
	target_pdb_path = os.path.join (out_path,
		'DSpellCheck_{}.pdb'.format (arch))
	if not os.path.isdir (out_path):
		os.makedirs (out_path)
	print ('Deploying to {}...'.format (target_zip_path))
	if os.path.isfile (target_zip_path):
		print ('Error: File {} already exists. Remove it explicitly if you want to overwrite.'
			.format (target_zip_path))
		sys.exit (1)
	zip (binary_path, target_zip_path)
	print ('Copying PDB...')
	shutil.copy (pdb_path, target_pdb_path)
	'''
if 'NPP_PLUGINS_X64_PATH' in os.environ:
	plugins_x64_path = os.environ['NPP_PLUGINS_X64_PATH']
	print ('Applying change to npp-plugins-x64 source directory...')
	plugins_xml_path = os.path.join (plugins_x64_path, 'plugins/plugins64.xml')
	plugins_xml_data = ''
	with open (plugins_xml_path, "r") as file:
		plugins_xml_data = file.read()
	bs = BeautifulSoup(plugins_xml_data, features="xml")
	plugin_node = bs.plugins.find("plugin", attrs={'name' : 'DSpellCheck'})
	ver = get_rc_version ()
	plugin_node.x64Version.string = ver
	plugin_node.download.string = 'https://github.com/Predelnik/DSpellCheck/releases/download/v{}/DSpellCheck_x64.zip'.arg (ver);
	print (plugin_node)
else:
	print ('%NPP_PLUGINS_X64_PATH% is not set up, nothing to update')

successString = 'Success!'
try:
	from colorama import init, Fore, Style
	init ()
	successString = Fore.GREEN + Style.BRIGHT + successString
except:
	pass
print (successString)