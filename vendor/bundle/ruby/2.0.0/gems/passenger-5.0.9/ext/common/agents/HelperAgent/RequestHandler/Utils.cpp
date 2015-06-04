/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2011-2015 Phusion
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

// This file is included inside the RequestHandler class.

#define RH_BENCHMARK_POINT(client, req, value) \
	do { \
		if (OXT_UNLIKELY(benchmarkMode == value)) { \
			writeBenchmarkResponse(&client, &req); \
			return; \
		} \
	} while (false)

private:

static TurboCaching<Request>::State
getTurboCachingInitialState(const VariantMap *agentsOptions) {
	bool enabled = agentsOptions->getBool("turbocaching", false, true);
	if (enabled) {
		return TurboCaching<Request>::ENABLED;
	} else {
		return TurboCaching<Request>::DISABLED;
	}
}

void
generateServerLogName(unsigned int number) {
	string name = "ServerThr." + toString(number);
	serverLogName = psg_pstrdup(stringPool, name);
}

void
disconnectWithClientSocketWriteError(Client **client, int e) {
	stringstream message;
	PassengerLogLevel logLevel;
	message << "client socket write error: ";
	message << ServerKit::getErrorDesc(e);
	message << " (errno=" << e << ")";
	if (e == EPIPE || e == ECONNRESET) {
		logLevel = LVL_INFO;
	} else {
		logLevel = LVL_WARN;
	}
	disconnectWithError(client, message.str(), logLevel);
}

void
disconnectWithAppSocketIncompleteResponseError(Client **client) {
	disconnectWithError(client, "application did not send a complete response");
}

void
disconnectWithAppSocketReadError(Client **client, int e) {
	stringstream message;
	message << "app socket read error: ";
	message << ServerKit::getErrorDesc(e);
	message << " (errno=" << e << ")";
	disconnectWithError(client, message.str());
}

void
disconnectWithAppSocketWriteError(Client **client, int e) {
	stringstream message;
	message << "app socket write error: ";
	message << ServerKit::getErrorDesc(e);
	message << " (errno=" << e << ")";
	disconnectWithError(client, message.str());
}

void
endRequestWithAppSocketIncompleteResponse(Client **client, Request **req) {
	if (!(*req)->responseBegun) {
		SKC_WARN(*client, "Sending 502 response: application did not send a complete response");
		endRequestWithSimpleResponse(client, req,
			"<h2>Incomplete response received from application</h2>", 502);
	} else {
		disconnectWithAppSocketIncompleteResponseError(client);
	}
}

void
endRequestWithAppSocketReadError(Client **client, Request **req, int e) {
	Client *c = *client;
	if (!(*req)->responseBegun) {
		SKC_WARN(*client, "Sending 502 response: application socket read error");
		endRequestWithSimpleResponse(client, req, "<h2>Application socket read error</h2>", 502);
	} else {
		disconnectWithAppSocketReadError(&c, e);
	}
}

/**
 * `data` must outlive the request.
 */
void
endRequestWithSimpleResponse(Client **c, Request **r, const StaticString &body, int code = 200) {
	Client *client = *c;
	Request *req = *r;
	ServerKit::HeaderTable headers;

	headers.insert(req->pool, "cache-control", "no-cache, no-store, must-revalidate");
	writeSimpleResponse(client, code, &headers, body);
	endRequest(c, r);
}

void
endRequestAsBadGateway(Client **client, Request **req) {
	if ((*req)->responseBegun) {
		disconnectWithError(client, "bad gateway");
	} else {
		ServerKit::HeaderTable headers;
		headers.insert((*req)->pool, "cache-control", "no-cache, no-store, must-revalidate");
		writeSimpleResponse(*client, 502, &headers, "<h1>Bad Gateway</h1>");
		endRequest(client, req);
	}
}

void
writeBenchmarkResponse(Client **client, Request **req, bool end = true) {
	if (canKeepAlive(*req)) {
		writeResponse(*client, P_STATIC_STRING(
			"HTTP/1.1 200 OK\r\n"
			"Status: 200 OK\r\n"
			"Date: Wed, 15 Nov 1995 06:25:24 GMT\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 3\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			"ok\n"));
	} else {
		writeResponse(*client, P_STATIC_STRING(
			"HTTP/1.1 200 OK\r\n"
			"Status: 200 OK\r\n"
			"Date: Wed, 15 Nov 1995 06:25:24 GMT\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 3\r\n"
			"Connection: close\r\n"
			"\r\n"
			"ok\n"));
	}
	if (end && !(*req)->ended()) {
		endRequest(client, req);
	}
}

bool
getBoolOption(Request *req, const HashedStaticString &name, bool defaultValue = false) {
	const LString *value = req->secureHeaders.lookup(name);
	if (value != NULL && value->size > 0) {
		return psg_lstr_first_byte(value) == 't';
	} else {
		return defaultValue;
	}
}

template<typename Number>
static Number
clamp(Number value, Number min, Number max) {
	return std::max(std::min(value, max), min);
}

static void
gatherBuffers(char * restrict dest, unsigned int size, const struct iovec *buffers,
	unsigned int nbuffers)
{
	const char *end = dest + size;
	char *pos = dest;

	for (unsigned int i = 0; i < nbuffers; i++) {
		assert(pos + buffers[i].iov_len <= end);
		memcpy(pos, buffers[i].iov_base, buffers[i].iov_len);
		pos += buffers[i].iov_len;
	}
}

// `path` MUST be NULL-terminated. Returns a contiguous LString.
static LString *
resolveSymlink(const StaticString &path, psg_pool_t *pool) {
	char linkbuf[PATH_MAX + 1];
	ssize_t size;

	size = readlink(path.data(), linkbuf, PATH_MAX);
	if (size == -1) {
		if (errno == EINVAL) {
			return psg_lstr_create(pool, path);
		} else {
			int e = errno;
			string message = "Cannot resolve possible symlink '";
			message.append(path.data(), path.size());
			message.append("'");
			throw FileSystemException(message, e, path.data());
		}
	} else {
		linkbuf[size] = '\0';
		if (linkbuf[0] == '\0') {
			string message = "The file '";
			message.append(path.data(), path.size());
			message.append("' is a symlink, and it refers to an empty filename. This is not allowed.");
			throw FileSystemException(message, ENOENT, path.data());
		} else if (linkbuf[0] == '/') {
			// Symlink points to an absolute path.
			size_t len = strlen(linkbuf);
			char *data = (char *) psg_pnalloc(pool, len + 1);
			memcpy(data, linkbuf, len);
			data[len] = '\0';
			return psg_lstr_create(pool, data, len);
		} else {
			// Symlink points to a relative path.
			// We do not use absolutizePath() because it's too slow.
			// This version doesn't handle all the edge cases but is
			// much faster.
			StaticString workingDir = extractDirNameStatic(path);
			size_t linkbuflen = strlen(linkbuf);
			size_t resultlen = linkbuflen + 1 + workingDir.size();
			char *data = (char *) psg_pnalloc(pool, resultlen);
			char *pos  = data;
			char *end  = data + resultlen;

			pos = appendData(pos, end, linkbuf, linkbuflen);
			*pos = '/';
			pos++;
			pos = appendData(pos, end, workingDir);

			return psg_lstr_create(pool, data, resultlen);
		}
	}
}

void
parseCookieHeader(psg_pool_t *pool, const LString *headerValue,
	vector< pair<StaticString, StaticString> > &cookies) const
{
	// See http://stackoverflow.com/questions/6108207/definite-guide-to-valid-cookie-values
	// for syntax grammar.
	vector<StaticString> parts;
	vector<StaticString>::const_iterator it, it_end;

	assert(headerValue->size > 0);
	headerValue = psg_lstr_make_contiguous(headerValue, pool);
	split(StaticString(headerValue->start->data, headerValue->size),
		';', parts);
	cookies.reserve(parts.size());
	it_end = parts.end();

	for (it = parts.begin(); it != it_end; it++) {
		const char *begin = it->data();
		const char *end = it->data() + it->size();
		const char *sep;

		skipLeadingWhitespaces(&begin, end);
		skipTrailingWhitespaces(begin, &end);

		// Find the separator ('=').
		sep = (const char *) memchr(begin, '=', end - begin);
		if (sep != NULL) {
			// Valid cookie. Otherwise, ignore it.
			const char *nameEnd = sep;
			const char *valueBegin = sep + 1;

			skipTrailingWhitespaces(begin, &nameEnd);
			skipLeadingWhitespaces(&valueBegin, end);

			cookies.push_back(make_pair(
				StaticString(begin, nameEnd - begin),
				StaticString(valueBegin, end - valueBegin)
			));
		}
	}
}

#ifdef DEBUG_RH_EVENT_LOOP_BLOCKING
	void
	reportLargeTimeDiff(Client *client, const char *name,
		ev_tstamp fromTime, ev_tstamp toTime)
	{
		if (fromTime != 0 && toTime != 0) {
			ev_tstamp blockTime = toTime - fromTime;
			if (blockTime > 0.01) {
				char buf[1024];
				int size = snprintf(buf, sizeof(buf), "%s: %.1f msec",
					name, blockTime * 1000);
				if (client != NULL) {
					SKC_NOTICE(client, StaticString(buf, size));
				} else {
					SKS_NOTICE(StaticString(buf, size));
				}
			}
		}
	}
#endif
