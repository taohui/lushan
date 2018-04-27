#!/usr/bin/python

import os
import sys
import shutil
import time
import datetime
import fcntl
import getopt 
from pwd import getpwnam
import ConfigParser

def getpwd():
	pwd = sys.path[0]
	if os.path.isfile(pwd):
		pwd = os.path.dirname(pwd)

	return pwd

def rmdir(top):
	for e in os.listdir(top):
		if os.path.isfile(os.path.join(top, e)):
			os.remove(os.path.join(top, e))

def move(src, dest):
	cmd = "mv %s %s"%(src, dest)
	return os.system(cmd)

def usage():
	print "--common_conf=<conf>"
	print "--conf=<conf>"
	print "--nolock  without lock"
	print "--local  without hadoop"
	print "--check_times=<check_times>"
	print "--check_interval=<check_interval>"

if __name__ == "__main__":

	nolock = False
	local = False
	common_config_file = ""
	config_file = ""
	check_interval = -1 
	check_times = -1
	opts, args = getopt.getopt(sys.argv[1:], "i:t:c:", ["nolock", "local", "common_conf=", "conf=", "check_interval=", "check_times="])
	for o, a in opts:
		if o == "--nolock":
			nolock = True
		elif o == "--local":
			local = True
		elif o == "--common_conf":
			common_config_file = a
		elif o in ("-c", "--conf"):
			config_file = a
		elif o in ("-i", "--check_interval"):
			check_interval = int(a)
		elif o in ("-t", "--check_times"):
			check_times = int(a)
		else:
			assert False, "unhandled option"
	if config_file == "":
		usage()
		sys.exit(0)

	if common_config_file == "":
		common_config_file = os.path.join(getpwd(), "../conf/common.conf")

	lock_file = os.path.join(getpwd(), "../conf/lock")

	common_config = ConfigParser.RawConfigParser()
	common_config.read(common_config_file)

	config = ConfigParser.RawConfigParser()
	config.read(config_file)

	java_home = common_config.get("DEFAULT", "JAVA_HOME")
	hadoop_home = common_config.get("DEFAULT", "HADOOP_HOME")
	rsync_module = common_config.get("DEFAULT", "RSYNC_MODULE")
	assert(rsync_module != "")


	local_dir = config.get("DEFAULT", "LOCAL_DIR")
	assert(local_dir != "")
	fileno = config.get("DEFAULT", "FILENO")
	destno = config.get("DEFAULT", "DESTNO")
	host_groupnos = config.get("DEFAULT", "HOST_GROUPNO").split("\t")
	user = config.get("DEFAULT", "USER")
	group = config.get("DEFAULT", "GROUP")

	command_prefix = ""
	if os.geteuid() == 0:
		command_prefix = "sudo -u %s JAVA_HOME=%s "%(user, java_home)

	local_tmp_dir = os.path.join(local_dir, fileno)
	print local_tmp_dir
	if os.path.exists(local_tmp_dir):
		shutil.rmtree(local_tmp_dir)
	os.mkdir(local_tmp_dir)
	os.chown(local_tmp_dir, getpwnam(user).pw_uid, getpwnam(user).pw_gid)

	file = "part-%05d"%int(fileno)

	if check_interval < 0:
		check_interval = config.getint("DEFAULT", "CHECK_INTERVAL")

	if check_times < 0:
		check_times = config.getint("DEFAULT", "CHECK_TIMES")

	i = 0
	ready = False
	while(i < check_times):
		if local:
			cmd = "/bin/ls -l %s/%s"%(config.get("DEFAULT", "LOCAL_RESULT_DIR"), file)
		else:
			cmd = "%s%s/bin/hadoop fs -ls %s/%s"%\
				(command_prefix, hadoop_home, \
				config.get("DEFAULT", "HDFS_RESULT_DIR"), file)
		print cmd
		ret = os.system(cmd)
		if ret == 0:
			ready = True
			break
		else:
			time.sleep(check_interval)
		i = i + 1
	if not ready:
		sys.exit(1)

	if not nolock:
		fl = open(lock_file, 'w')
		fcntl.flock(fl, fcntl.LOCK_EX)

	if not local:
		cmd = "%s%s/bin/hadoop fs -copyToLocal -ignoreCrc %s/%s %s/"% \
			(command_prefix, hadoop_home, \
			config.get("DEFAULT", "HDFS_RESULT_DIR"), file, local_tmp_dir)	

		print cmd
		ret = os.system(cmd)
		if ret != 0:
			if not nolock:
				fcntl.flock(fl, fcntl.LOCK_UN)
				fl.close()
			sys.exit(1)

	hdict = "hdict_%s"%datetime.datetime.now().strftime("%Y%m%d%H%M%S")
	hdict_path = os.path.join(local_tmp_dir, hdict)
	if local:
		cmd = "%s %s %s"%(config.get("DEFAULT", "TRANSFORM_PLUGIN"), \
			os.path.join(config.get("DEFAULT", "LOCAL_RESULT_DIR"), file), hdict_path)
	else:
		cmd = "%s %s %s"%(config.get("DEFAULT", "TRANSFORM_PLUGIN"), \
			os.path.join(local_tmp_dir, file), hdict_path)
	print cmd
	ret = os.system(cmd)
	if ret != 0:
		if not nolock:
			fcntl.flock(fl, fcntl.LOCK_UN)
			fl.close()
		sys.exit(1)

	contain_local = False
	for host_groupno in host_groupnos:
		hosts = common_config.get("HOST_GROUP_%s"%host_groupno, "SERVERS").split(",")
		for host in hosts:
			if host == "127.0.0.1":
				contain_local = True
			else:
				cmd = "rsync -a --bwlimit=20000 %s %s::%s/%s/"% \
				(hdict_path, host, rsync_module, destno)
				print cmd
				ret = os.system(cmd)
				if ret != 0:
					if not nolock:
						fcntl.flock(fl, fcntl.LOCK_UN)
						fl.close()
					sys.exit(1)

	if contain_local:
		lushan_upload_dir = os.path.join(getpwd(), "../../lushan/upload")
		move(hdict_path, "%s/%s/"%(lushan_upload_dir, destno))

	done_flg_path = os.path.join(local_tmp_dir, "done.flg")
	cmd = "touch %s"%done_flg_path
	ret = os.system(cmd)
	if ret != 0:
		if not nolock:
			fcntl.flock(fl, fcntl.LOCK_UN)
			fl.close()
		sys.exit(1)

	for host_groupno in host_groupnos:
		hosts = common_config.get("HOST_GROUP_%s"%host_groupno, "SERVERS").split(",")
		for host in hosts:
			if host != "127.0.0.1":
				cmd = "rsync -a --bwlimit=20000 %s %s::%s/%s/%s/"% \
				(done_flg_path, host, rsync_module, destno, hdict)
				print cmd
				ret = os.system(cmd)
				if ret != 0:
					if not nolock:
						fcntl.flock(fl, fcntl.LOCK_UN)
						fl.close()
					sys.exit(1)
	if contain_local:
		lushan_upload_dir = os.path.join(getpwd(), "../../lushan/upload")
		move(done_flg_path, "%s/%s/%s/"%(lushan_upload_dir, destno, hdict))

	shutil.rmtree(local_tmp_dir)

	if not nolock:
		fcntl.flock(fl, fcntl.LOCK_UN)
		fl.close()

	sys.exit(0)
