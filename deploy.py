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
from pygit2 import Repository, Config, Signature, GIT_OBJ_COMMIT

def get_version_number (filename):
	info = GetFileVersionInfo (filename, "\\")
	ms = info['FileVersionMS']
	ls = info['FileVersionLS']
	return '{}.{}.{}.{}'.format (HIWORD (ms), LOWORD (ms), HIWORD (ls), LOWORD (ls))

version_str = re.compile (r'(\s*VALUE "FileVersion", ")([0-9.]*)(")')
script_dir = os.path.dirname(os.path.realpath(__file__))

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
parser.add_argument('--update-only-rc', action="store_true", dest="update_only_rc", help="Update only resource files if build should be done later", default=False)
parser.add_argument('--add-tag', action="store_true", dest="add_tag", help="Add tag for version", default=False)
parser.add_argument('-v', '--verbose', action="store_true", dest="verbose", help="Verbose output", default=False)
options = parser.parse_args()

ver = get_rc_version ().split ('.')
ver_str = '.'.join (ver)

def create_commit(repo, msg):
	index = repo.index
	index.add_all ()
	index.write ()
	config = Config.get_global_config()
	author = Signature(config['user.name'], config['user.email'])
	commiter = author
	tree = index.write_tree()
	repo.create_commit ('HEAD', author, commiter, msg, tree, [repo.head.get_object().hex])

def add_version_commit():
	repo = Repository (script_dir)
	create_commit (repo, 'Update to {}'.format (ver_str))
	config = Config.get_global_config()
	author = Signature(config['user.name'], config['user.email'])

def add_version_tag ():
	repo = Repository (script_dir)
	author = Signature(config['user.name'], config['user.email'])
	repo.create_tag('v{}'.format (ver_str), repo.revparse_single('HEAD').id, GIT_OBJ_COMMIT, author, 'v{}'.format (ver_str))

new_ver_is_added = False
if options.new_minor:
	ver[-1]=str (int (ver[-1]) + 1)
	new_ver_is_added = True
if options.new_major:
	ver = get_rc_version ().split ('.')
	ver[-2]=str (int (ver[-2]) + 1)
	ver[-1]='0'
	new_ver_is_added=True

if new_ver_is_added:
	replace_rc_version (ver)
	ver_str = '.'.join (ver)
	print ('Version increased to {}'.format (ver_str))
	add_version_commit()

if options.update_only_rc:
	exit(0)

if new_ver_is_added or options.add_tag:
	add_version_tag()

x64_binary_path = ''
x86_binary_path = ''
x64_zip_path = ''
x86_zip_path = ''

for arch in ['x64', 'x86']:
	dir = 'build-deploy-msvc2019-{}'.format (arch)
	FNULL = open(os.devnull, 'w')
	if call(['cmake', '..'], stdout= (None if options.verbose else FNULL), cwd=dir) != 0:
		print ('Error: Cmake error')
		sys.exit (1)
	build_args = ['cmake', '--build', dir, '--config', 'RelWithDebInfo']
	print ('Building {} version...'.format (arch))
	binary_path = os.path.join (dir, 'RelWithDebInfo', 'DSpellCheck.dll')
	if arch == 'x64':
		x64_binary_path = binary_path
	else:
		x86_binary_path = binary_path
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
	if arch == 'x64':
		x64_zip_path = target_zip_path
	else:
		x86_zip_path = target_zip_path
	print ('Copying PDB...')
	shutil.copy (pdb_path, target_pdb_path)

if options.new_minor or options.update_pm:
	ver = get_rc_version ()

	if  'NPP_PLUGIN_LIST_PATH' in os.environ:
		plugin_list_path = os.environ['NPP_PLUGIN_LIST_PATH']
		print ('Applying change to nppPluginList source directory...')
		for t in [('src/pl.x64.json', x64_zip_path, 'x64'), ('src/pl.x86.json', x86_zip_path, 'x86')]:
			plugins_json_path = os.path.join (plugin_list_path, t[0])
			json_data = None
			with open(plugins_json_path, encoding='utf-8') as data_file:
				json_data = json.loads(data_file.read ())
			plugin_node = None
			for node in json_data["npp-plugins"]:
				if node["display-name"] == "DSpellCheck":
					plugin_node = node
					break
			plugin_node["version"] = ver
			plugin_node["id"] = hashlib.sha256(open(t[1], 'rb').read()).hexdigest()
			plugin_node["repository"] = 'https://github.com/Predelnik/DSpellCheck/releases/download/v{}/DSpellCheck_{}.zip'.format (ver, t[2])
			with open (plugins_json_path, "w", encoding='utf-8') as fp:
				json.dump (json_data, fp, indent='\t', ensure_ascii=False)
				fp.write ('\n')

		print ('Creating commit in nppPluginList repository...')
		create_commit (Repository(plugin_list_path), 'Update DSpellCheck to {}'.format (ver))
	else:
		print ('%NPP_PLUGIN_LIST_PATH% is not set up, nothing to update')

successString = 'Success!'
try:
	from colorama import init, Fore, Style
	init ()
	successString = Fore.GREEN + Style.BRIGHT + successString
except:
	pass
print (successString)
