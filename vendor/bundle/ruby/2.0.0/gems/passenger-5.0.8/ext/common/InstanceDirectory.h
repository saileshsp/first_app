/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2014 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#ifndef _PASSENGER_INSTANCE_DIRECTORY_H_
#define _PASSENGER_INSTANCE_DIRECTORY_H_

#include <boost/shared_ptr.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cassert>
#include <string>
#include <Constants.h>
#include <Exceptions.h>
#include <RandomGenerator.h>
#include <Utils.h>
#include <Utils/IOUtils.h>
#include <Utils/json.h>

namespace Passenger {

using namespace std;


class InstanceDirectory {
public:
	struct CreationOptions {
		string prefix;
		bool userSwitching;
		uid_t defaultUid;
		gid_t defaultGid;
		Json::Value properties;

		CreationOptions()
			: prefix("passenger"),
			  userSwitching(true),
			  defaultUid(USER_NOT_GIVEN),
			  defaultGid(GROUP_NOT_GIVEN)
			{ }
	};

private:
	const string path;
	bool owner;

	static string createUniquePath(const string &registryDir, const string &prefix) {
		RandomGenerator generator;

		for (int i = 0; i < 250; i++) {
			string suffix = generator.generateAsciiString(7);
			string path = registryDir + "/" + prefix + "." + suffix;
			if (createPath(registryDir, path)) {
				return path;
			}
		}
		throw RuntimeException("Unable to create a unique directory inside "
			" instance registry directory " + registryDir + ", even after 250 tries");
	}

	static bool createPath(const string &registryDir, const string &path) {
		if (mkdir(path.c_str(), parseModeString("u=rwx,g=rx,o=rx")) == -1) {
			if (errno == EEXIST) {
				return false;
			} else {
				int e = errno;
				throw FileSystemException("Cannot create a subdirectory inside"
					" instance registry directory " + registryDir, e, registryDir);
			}
		}
		// Explicitly chmod the directory in case the umask is interfering.
		if (chmod(path.c_str(), parseModeString("u=rwx,g=rx,o=rx")) == -1) {
			int e = errno;
			throw FileSystemException("Cannot set permissions on instance directory " +
				path, e, path);
		}
		// The parent directory may have the setgid bit enabled, so we
		// explicitly chown it.
		if (chown(path.c_str(), geteuid(), getegid()) == -1) {
			int e = errno;
			throw FileSystemException("Cannot change the permissions of the instance "
				"directory " + path, e, path);
		}
		return true;
	}

	void initializeInstanceDirectory(const CreationOptions &options) {
		createPropertyFile(options);
		createAgentSocketsSubdir();
		createAppSocketsSubdir(options);
		createLockFile();
	}

	bool runningAsRoot() const {
		return geteuid() == 0;
	}

	void createAgentSocketsSubdir() {
		if (runningAsRoot()) {
			/* The server socket must be accessible by the web server
			 * and by the apps, which may run as complete different users,
			 * so this subdirectory must be world-accessible.
			 */
			makeDirTree(path + "/agents.s", "u=rwx,g=rx,o=rx");
		} else {
			makeDirTree(path + "/agents.s", "u=rwx,g=,o=");
		}
	}

	void createAppSocketsSubdir(const CreationOptions &options) {
		if (runningAsRoot()) {
			if (options.userSwitching) {
				/* Each app may be running as a different user,
				 * so the apps.s subdirectory must be world-writable.
				 * However we don't want everybody to be able to know the
				 * sockets' filenames, so the directory is not readable.
				 */
				makeDirTree(path + "/apps.s", "u=rwx,g=wx,o=wx,+t");
			} else {
				/* All apps are running as defaultUser/defaultGroup,
				 * so make defaultUser/defaultGroup the owner and group of the
				 * subdirecory.
				 *
				 * The directory is not readable as a security precaution:
				 * nobody should be able to know the sockets' filenames without
				 * having access to the application pool.
				 */
				makeDirTree(path + "/apps.s", "u=rwx,g=x,o=x",
					options.defaultUid, options.defaultGid);
			}
		} else {
			/* All apps are running as the same user as the web server,
			 * so only allow access for this user.
			 */
			makeDirTree(path + "/apps.s", "u=rwx,g=,o=");
		}
	}

	void createPropertyFile(const CreationOptions &options) {
		Json::Value props;

		props["instance_dir"]["major_version"] = SERVER_INSTANCE_DIR_STRUCTURE_MAJOR_VERSION;
		props["instance_dir"]["minor_version"] = SERVER_INSTANCE_DIR_STRUCTURE_MINOR_VERSION;
		props["instance_dir"]["created_at"] = (long long) time(NULL);
		props["passenger_version"] = PASSENGER_VERSION;
		props["watchdog_pid"] = (Json::UInt64) getpid();

		Json::Value::Members members = options.properties.getMemberNames();
		Json::Value::Members::const_iterator it, end = members.end();
		for (it = members.begin(); it != end; it++) {
			props[*it] = options.properties.get(*it, Json::Value());
		}

		createFile(path + "/properties.json", props.toStyledString());
	}

	void createLockFile() {
		createFile(path + "/lock", "");
	}

public:
	InstanceDirectory(const CreationOptions &options)
		: path(createUniquePath(getSystemTempDir(), options.prefix)),
		  owner(true)
	{
		initializeInstanceDirectory(options);
	}

	InstanceDirectory(const CreationOptions &options, const string &registryDir)
		: path(createUniquePath(registryDir, options.prefix)),
		  owner(true)
	{
		initializeInstanceDirectory(options);
	}

	InstanceDirectory(const string &dir)
		: path(dir),
		  owner(false)
		{ }

	~InstanceDirectory() {
		if (owner) {
			destroy();
		}
	}

	void finalizeCreation() {
		assert(owner);
		createFile(path + "/creation_finalized", "");
	}

	// The 'const string &' here is on purpose. The AgentsStarter C
	// functions return the string pointer directly.
	const string &getPath() const {
		return path;
	}

	void detach() {
		owner = false;
	}

	bool isOwner() const {
		return owner;
	}

	void destroy() {
		assert(owner);
		owner = false;
		removeDirTree(path);
	}
};

typedef boost::shared_ptr<InstanceDirectory> InstanceDirectoryPtr;


} // namespace Passenger

#endif /* _PASSENGER_INSTANCE_DIRECTORY_H_ */
