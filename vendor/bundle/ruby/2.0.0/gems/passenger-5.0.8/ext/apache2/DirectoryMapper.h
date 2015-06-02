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
#ifndef _PASSENGER_DIRECTORY_MAPPER_H_
#define _PASSENGER_DIRECTORY_MAPPER_H_

#include <string>
#include <set>
#include <cstring>

#include <oxt/backtrace.hpp>
#include <boost/thread.hpp>

#include "Configuration.hpp"
#include <ApplicationPool2/AppTypes.h>
#include <Utils.h>
#include <Utils/CachedFileStat.hpp>

// The Apache/APR headers *must* come after the Boost headers, otherwise
// compilation will fail on OpenBSD.
#include <httpd.h>
#include <http_core.h>

namespace Passenger {

using namespace std;
using namespace oxt;
using namespace Passenger::ApplicationPool2;


class DocumentRootDeterminationError: public oxt::tracable_exception {
private:
	string msg;
public:
	DocumentRootDeterminationError(const string &message): msg(message) {}
	virtual ~DocumentRootDeterminationError() throw() {}
	virtual const char *what() const throw() { return msg.c_str(); }
};


/**
 * Utility class for determining URI-to-application directory mappings.
 * Given a URI, it will determine whether that URI belongs to a Phusion
 * Passenger-handled application, what the base URI of that application is,
 * and what the associated 'public' directory is.
 *
 * @note This class is not thread-safe, but is reentrant.
 * @ingroup Core
 */
class DirectoryMapper {
private:
	DirConfig *config;
	request_rec *r;
	CachedFileStat *cstat;
	boost::mutex *cstatMutex;
	const char *baseURI;
	string publicDir;
	string appRoot;
	unsigned int throttleRate;
	PassengerAppType appType: 7;
	bool autoDetectionDone: 1;

	const char *findBaseURI() const {
		set<string>::const_iterator it, end = config->baseURIs.end();
		const char *uri = r->uri;
		size_t uri_len = strlen(uri);

		for (it = config->baseURIs.begin(); it != end; it++) {
			const string &base = *it;

			if (base == "/") {
                /* Ignore 'PassengerBaseURI /' options. Users usually
                 * specify this out of ignorance.
                 */
                continue;
            }

			if (  base == "/"
			 || ( uri_len == base.size() && memcmp(uri, base.c_str(), uri_len) == 0 )
			 || ( uri_len  > base.size() && memcmp(uri, base.c_str(), base.size()) == 0
			                             && uri[base.size()] == '/' )
			) {
				return base.c_str();
			}
		}
		return NULL;
	}

	/**
	 * @throws FileSystemException An error occured while examening the filesystem.
	 * @throws DocumentRootDeterminationError Unable to query the location of the document root.
	 * @throws TimeRetrievalException
	 * @throws boost::thread_interrupted
	 */
	void autoDetect() {
		if (autoDetectionDone) {
			return;
		}

		TRACE_POINT();

		/* Determine the document root without trailing slashes. */
		StaticString docRoot = ap_document_root(r);
		if (docRoot.size() > 1 && docRoot[docRoot.size() - 1] == '/') {
			docRoot = docRoot.substr(0, docRoot.size() - 1);
		}
		if (docRoot.empty()) {
			throw DocumentRootDeterminationError("Cannot determine the document root");
		}

		/* Find the base URI for this web application, if any. */
		const char *baseURI = findBaseURI();
		if (baseURI != NULL) {
			/* We infer that the 'public' directory of the web application
			 * is document root + base URI.
			 */
			publicDir = docRoot + baseURI;
		} else {
			/* No base URI directives are applicable for this request. So assume that
			 * the web application's public directory is the document root.
			 */
			publicDir = docRoot;
		}

		UPDATE_TRACE_POINT();
		AppTypeDetector detector(cstat, cstatMutex, throttleRate);
		PassengerAppType appType;
		string appRoot;
		if (config->appType == NULL) {
			if (config->appRoot == NULL) {
				appType = detector.checkDocumentRoot(publicDir,
					baseURI != NULL || config->resolveSymlinksInDocRoot == DirConfig::ENABLED,
					&appRoot);
			} else {
				appRoot = config->appRoot;
				appType = detector.checkAppRoot(appRoot);
			}
		} else {
			if (config->appRoot == NULL) {
				appType = PAT_NONE;
			} else {
				appRoot = config->appRoot;
				appType = getAppType(config->appType);
			}
		}

		this->appRoot = appRoot;
		this->baseURI = baseURI;
		this->appType = appType;
		autoDetectionDone = true;
	}

public:
	/**
	 * Create a new DirectoryMapper object.
	 *
	 * @param cstat A CachedFileStat object used for statting files.
	 * @param cstatMutex A mutex for locking CachedFileStat, making its
	 *                   usage thread-safe.
	 * @param throttleRate A throttling rate for cstat.
	 * @warning Do not use this object after the destruction of <tt>r</tt>,
	 *          <tt>config</tt> or <tt>cstat</tt>.
	 */
	DirectoryMapper(request_rec *r, DirConfig *config, CachedFileStat *cstat,
	                boost::mutex *cstatMutex, unsigned int throttleRate) {
		this->r = r;
		this->config = config;
		this->cstat = cstat;
		this->cstatMutex = cstatMutex;
		this->throttleRate = throttleRate;
		appType = PAT_NONE;
		baseURI = NULL;
		autoDetectionDone = false;
	}

	/**
	 * Determines whether the given HTTP request falls under one of the specified
	 * PassengerBaseURIs. If yes, then the first matching base URI will be returned.
	 * Otherwise, NULL will be returned.
	 *
	 * @throws FileSystemException An error occured while examening the filesystem.
	 * @throws DocumentRoot
	 * @throws TimeRetrievalException
	 * @throws boost::thread_interrupted
	 * @warning The return value may only be used as long as <tt>config</tt>
	 *          hasn't been destroyed.
	 */
	const char *getBaseURI() {
		TRACE_POINT();
		autoDetect();
		return baseURI;
	}

	/**
	 * Returns the filename of the 'public' directory of the application
	 * that's associated with the HTTP request.
	 *
	 * @throws FileSystemException An error occured while examening the filesystem.
	 * @throws DocumentRootDeterminationError Unable to query the location of the document root.
	 * @throws TimeRetrievalException
	 * @throws boost::thread_interrupted
	 */
	const string &getPublicDirectory() {
		autoDetect();
		return publicDir;
	}

	/**
	 * Returns the application root, or the empty string if this request does not
	 * belong to an application.
	 *
	 * @throws FileSystemException An error occured while examening the filesystem.
	 * @throws DocumentRootDeterminationError Unable to query the location of the document root.
	 * @throws TimeRetrievalException
	 * @throws boost::thread_interrupted
	 */
	const string &getAppRoot() {
		autoDetect();
		return appRoot;
	}

	/**
	 * Returns the application type that's associated with the HTTP request.
	 *
	 * @throws FileSystemException An error occured while examening the filesystem.
	 * @throws DocumentRootDeterminationError Unable to query the location of the document root.
	 * @throws TimeRetrievalException
	 * @throws boost::thread_interrupted
	 */
	PassengerAppType getApplicationType() {
		autoDetect();
		return appType;
	}

	/**
	 * Returns the application type (as a string) that's associated
	 * with the HTTP request.
	 *
	 * @throws FileSystemException An error occured while examening the filesystem.
	 * @throws DocumentRootDeterminationError Unable to query the location of the document root.
	 * @throws TimeRetrievalException
	 * @throws boost::thread_interrupted
	 */
	const char *getApplicationTypeName() {
		autoDetect();
		return getAppTypeName(appType);
	}
};

} // namespace Passenger

#endif /* _PASSENGER_DIRECTORY_MAPPER_H_ */

