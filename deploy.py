from subprocess import call
import subprocess
from win32api import GetFileVersionInfo, LOWORD, HIWORD
import zipfile
import os
import argparse
import re
import sys
import shutil
import json
import hashlib
from bs4 import BeautifulSoup
import datetime

def get_version_number (filename):
	info = GetFileVersionInfo (filename, "\\")
	ms = info['FileVersionMS']
	ls = info['FileVersionLS']
	return '{}.{}.{}.{}'.format (HIWORD (ms), LOWORD (ms), HIWORD (ls), LOWORD (ls))

version_str = re.compile (r'(\s*VALUE "FileVersion", ")([0-9.]*)(")')

def get_rc_version ():
	with open('src/DSpellCheck.rc', encoding='utf-16-le') as f:
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
	with open(target_rc_path, 'w', encoding='utf-16-le') as fw, open(source_rc_path, 'r', encoding='utf-16-le') as f:
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
parser.add_argument('--new-major', action="store_true", dest="new_major", help="Deploy new major version", default=False)
parser.add_argument('--update-pm', action="store_true", dest="update_pm", help="Update plugin manager (done automatically if new minor version is set)", default=False)
parser.add_argument('-v', '--verbose', action="store_true", dest="verbose", help="Verbose output", default=False)
options = parser.parse_args()
if options.new_minor:
	ver = get_rc_version ().split ('.')
	ver[-1]=str (int (ver[-1]) + 1)
	replace_rc_version (ver)
	print ('Version increased to {}'.format ('.'.join (ver)))
if options.new_major:
	ver = get_rc_version ().split ('.')
	ver[-2]=str (int (ver[-2]) + 1)
	ver[-1]='0'
	replace_rc_version (ver)
	print ('Version increased to {}'.format ('.'.join (ver)))
x64_binary_path = ''

for arch in ['x64', 'x86']:
	dir = 'build-deploy-msvc2017-{}'.format (arch)
	FNULL = open(os.devnull, 'w')
	if call(['cmake', '..'], stdout= (None if options.verbose else FNULL), cwd=dir) != 0:
		print ('Error: Cmake error')
		sys.exit (1)
	build_args = ['cmake', '--build', dir, '--config', 'RelWithDebInfo']
	print ('Building {} version...'.format (arch))
	binary_path = os.path.join (dir, 'RelWithDebInfo', 'DSpellCheck.dll')
	if arch == 'x64':
		x64_binary_path = binary_path
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

if options.new_minor or options.update_pm:
	if 'NPP_PLUGINS_X64_PATH' in os.environ:
		plugins_x64_path = os.environ['NPP_PLUGINS_X64_PATH']
		print ('Applying change to npp-plugins-x64 source directory...')
		plugins_xml_path = os.path.join (plugins_x64_path, 'plugins/plugins64.xml')
		plugins_xml_data = ''
		with open (plugins_xml_path, "r") as file:
			plugins_xml_data = file.read()
		bs = BeautifulSoup(plugins_xml_data, features="xml")
		plugin_node = bs.plugins.find("plugin", attrs={'name' : 'DSpellCheck'})
		def replace_text(bs_node, new_text):
			prev_node_str = str (bs_node)
			bs_node.string = new_text
			global plugins_xml_data
			plugins_xml_data = plugins_xml_data.replace (prev_node_str, str (bs_node))
	
		ver = get_rc_version ()
		replace_text (plugin_node.x64Version, ver)
		replace_text (plugin_node.download, 'https://github.com/Predelnik/DSpellCheck/releases/download/v{}/DSpellCheck_x64.zip'.format (ver))
		readme_text = ''
		with open ('readme.md', "r") as file:
			readme_text = file.read()
		key = 'v{}'.format (ver)
		start_pos = readme_text.find (key)
		start_pos += len (key) + 1
		end_pos = readme_text.find ('\n\n', start_pos)
		replace_text (plugin_node.latestUpdate, datetime.datetime.now().strftime ("%Y-%m-%d") + '\\n' + readme_text[start_pos:end_pos].replace ('\n', '\\n') + '\\n')
		with open (plugins_xml_path, "w") as file:
			file.write (plugins_xml_data)
	
		validate_path = os.path.join (plugins_x64_path, 'plugins/validate.json')
		validate_data = None
		validate_text = ''
		with open(validate_path) as data_file:
			validate_text = data_file.read ()
			validate_data = json.loads(validate_text)
		str_before = json.dumps (validate_data['DSpellCheck'])
		validate_data['DSpellCheck'] = {}
		validate_data['DSpellCheck'][hashlib.md5(open(x64_binary_path, 'rb').read()).hexdigest()] = 'DSpellCheck.dll'
		str_after = json.dumps (validate_data['DSpellCheck'])
		validate_text = validate_text.replace (str_before[1:-1], str_after[1:-1])
		with open (validate_path, "w") as file:
			file.write (validate_text)
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