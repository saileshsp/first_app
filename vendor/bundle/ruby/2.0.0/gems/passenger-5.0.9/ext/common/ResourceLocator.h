/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2010-2014 Phusion
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
#ifndef _PASSENGER_RESOURCE_LOCATOR_H_
#define _PASSENGER_RESOURCE_LOCATOR_H_

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <pwd.h>
#include <Constants.h>
#include <Exceptions.h>
#include <Utils.h>
#include <Utils/IniFile.h>

namespace Passenger {

using namespace std;
using namespace boost;


/**
 * Locates various Phusion Passenger resources on the filesystem. All Phusion Passenger
 * files are located through this class. There's similar code in lib/phusion_passenger.rb.
 * See doc/Packaging.txt.md for an introduction about where Phusion Passenger expects its
 * files to be located.
 */
class ResourceLocator {
private:
	string root;
	string binDir;
	string supportBinariesDir;
	string helperScriptsDir;
	string resourcesDir;
	string docDir;
	string rubyLibDir;
	string nodeLibDir;

	static string getOption(const string &file, const IniFileSectionPtr &section, const string &key) {
		if (section->hasKey(key)) {
			return section->get(key);
		} else {
			throw RuntimeException("Option '" + key + "' missing in file '" + file + "'");
		}
	}

public:
	ResourceLocator() { }

	ResourceLocator(const string &rootOrFile) {
		root = rootOrFile;
		if (getFileType(rootOrFile) == FT_REGULAR) {
			string file = rootOrFile;
			IniFileSectionPtr options = IniFile(file).section("locations");
			binDir              = getOption(file, options, "bin_dir");
			supportBinariesDir  = getOption(file, options, "support_binaries_dir");
			helperScriptsDir    = getOption(file, options, "helper_scripts_dir");
			resourcesDir        = getOption(file, options, "resources_dir");
			docDir              = getOption(file, options, "doc_dir");
			rubyLibDir          = getOption(file, options, "ruby_libdir");
			nodeLibDir          = getOption(file, options, "node_libdir");
		} else {
			string root = rootOrFile;
			binDir              = root + "/bin";
			supportBinariesDir  = root + "/buildout/support-binaries";
			helperScriptsDir    = root + "/helper-scripts";
			resourcesDir        = root + "/resources";
			docDir              = root + "/doc";
			rubyLibDir          = root + "/lib";
			nodeLibDir          = root + "/node_lib";
		}
	}

	string getRoot() const {
		return root;
	}

	string getSupportBinariesDir() const {
		return supportBinariesDir;
	}

	string getUserSupportBinariesDir() const {
		struct passwd pwd, *user;
		long bufSize;
		shared_array<char> strings;

		// _SC_GETPW_R_SIZE_MAX is not a maximum:
		// http://tomlee.co/2012/10/problems-with-large-linux-unix-groups-and-getgrgid_r-getgrnam_r/
		bufSize = std::max<long>(1024 * 128, sysconf(_SC_GETPW_R_SIZE_MAX));
		strings.reset(new char[bufSize]);

		user = (struct passwd *) NULL;
		if (getpwuid_r(getuid(), &pwd, strings.get(), bufSize, &user) != 0) {
			user = (struct passwd *) NULL;
		}

		if (user == (struct passwd *) NULL) {
			int e = errno;
			throw SystemException("Cannot lookup system user database", e);
		}

		string result(user->pw_dir);
		result.append("/");
		result.append(USER_NAMESPACE_DIRNAME);
		result.append("/support-binaries/");
		result.append(PASSENGER_VERSION);
		return result;
	}

	string getHelperScriptsDir() const {
		return helperScriptsDir;
	}

	string getResourcesDir() const {
		return resourcesDir;
	}

	string getDocDir() const {
		return docDir;
	}

	// Can be empty.
	string getRubyLibDir() const {
		return rubyLibDir;
	}

	string getNodeLibDir() const {
		return nodeLibDir;
	}

	string findSupportBinary(const string &name) {
		string path = getSupportBinariesDir() + "/" + name;
		bool found;
		try {
			found = fileExists(path);
		} catch (const SystemException &e) {
			found = false;
		}
		if (found) {
			return path;
		}

		path = getUserSupportBinariesDir() + "/" + name;
		if (fileExists(path)) {
			return path;
		}

		throw RuntimeException("Support binary " + name + " not found (tried: " + getSupportBinariesDir() + "/" + name + " and " + path + ")");
	}
};

typedef boost::shared_ptr<ResourceLocator> ResourceLocatorPtr;


}

#endif /* _PASSENGER_RESOURCE_LOCATOR_H_ */
