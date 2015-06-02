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
#ifndef _PASSENGER_APPLICATION_POOL_SOCKET_H_
#define _PASSENGER_APPLICATION_POOL_SOCKET_H_

#include <vector>
#include <oxt/macros.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <climits>
#include <cassert>
#include <Logging.h>
#include <StaticString.h>
#include <ApplicationPool2/Common.h>
#include <Utils/SmallVector.h>
#include <Utils/IOUtils.h>

namespace Passenger {
namespace ApplicationPool2 {

using namespace std;
using namespace boost;


struct Connection {
	int fd;
	bool wantKeepAlive: 1;
	bool fail: 1;
	bool blocking: 1;

	Connection()
		: fd(-1),
		  wantKeepAlive(false),
		  fail(false),
		  blocking(true)
		{ }

	void close() {
		if (fd != -1) {
			int fd2 = fd;
			fd = -1;
			wantKeepAlive = false;
			safelyClose(fd2);
			P_LOG_FILE_DESCRIPTOR_CLOSE(fd2);
		}
	}
};

/**
 * Not thread-safe except for the connection pooling methods, so only use
 * within the ApplicationPool lock.
 */
class Socket {
private:
	boost::mutex connectionPoolLock;
	vector<Connection> idleConnections;

	OXT_FORCE_INLINE
	int connectionPoolLimit() const {
		return concurrency;
	}

	Connection connect() const {
		Connection connection;
		P_TRACE(3, "Connecting to " << address);
		connection.fd = connectToServer(address, __FILE__, __LINE__);
		connection.fail = true;
		connection.wantKeepAlive = false;
		connection.blocking = true;
		P_LOG_FILE_DESCRIPTOR_PURPOSE(connection.fd, "App " << pid << " connection");
		return connection;
	}

public:
	// Socket properties. Read-only.
	StaticString name;
	StaticString address;
	StaticString protocol;
	pid_t pid;
	int concurrency;

	// Private. In public section as alignment optimization.
	int totalConnections;
	int totalIdleConnections;

	/** Invariant: sessions >= 0 */
	int sessions;

	Socket()
		: pid(-1),
		  concurrency(0)
		{ }

	Socket(pid_t _pid, const StaticString &_name, const StaticString &_address,
		const StaticString &_protocol, int _concurrency)
		: name(_name),
		  address(_address),
		  protocol(_protocol),
		  pid(_pid),
		  concurrency(_concurrency),
		  totalConnections(0),
		  totalIdleConnections(0),
		  sessions(0)
		{ }

	Socket(const Socket &other)
		: idleConnections(other.idleConnections),
		  name(other.name),
		  address(other.address),
		  protocol(other.protocol),
		  pid(other.pid),
		  concurrency(other.concurrency),
		  totalConnections(other.totalConnections),
		  totalIdleConnections(other.totalIdleConnections),
		  sessions(other.sessions)
		{ }

	Socket &operator=(const Socket &other) {
		totalConnections = other.totalConnections;
		totalIdleConnections = other.totalIdleConnections;
		idleConnections = other.idleConnections;
		name = other.name;
		address = other.address;
		protocol = other.protocol;
		pid = other.pid;
		concurrency = other.concurrency;
		sessions = other.sessions;
		return *this;
	}

	/**
	 * Connect to this socket or reuse an existing connection.
	 *
	 * One MUST call checkinConnection() when one's done using the Connection.
	 * Failure to do so will result in a resource leak.
	 */
	Connection checkoutConnection() {
		boost::unique_lock<boost::mutex> l(connectionPoolLock);

		if (!idleConnections.empty()) {
			P_TRACE(3, "Socket " << address << ": checking out connection from connection pool (" <<
				idleConnections.size() << " -> " << (idleConnections.size() - 1) <<
				" items). Current total number of connections: " << totalConnections);
			Connection connection = idleConnections.back();
			idleConnections.pop_back();
			totalIdleConnections--;
			return connection;
		} else {
			Connection connection = connect();
			totalConnections++;
			P_TRACE(3, "Socket " << address << ": there are now " <<
				totalConnections << " total connections");
			l.unlock();
			return connection;
		}
	}

	void checkinConnection(Connection &connection) {
		boost::unique_lock<boost::mutex> l(connectionPoolLock);

		if (connection.fail || !connection.wantKeepAlive || totalIdleConnections >= connectionPoolLimit()) {
			totalConnections--;
			assert(totalConnections >= 0);
			P_TRACE(3, "Socket " << address << ": connection not checked back into "
				"connection pool. There are now " << totalConnections <<
				" connections in total");
			l.unlock();
			connection.close();
		} else {
			P_TRACE(3, "Socket " << address << ": checking in connection into connection pool (" <<
				totalIdleConnections << " -> " << (totalIdleConnections + 1) <<
				" items). Current total number of connections: " << totalConnections);
			totalIdleConnections++;
			idleConnections.push_back(connection);
		}
	}

	void closeAllConnections() {
		boost::unique_lock<boost::mutex> l(connectionPoolLock);
		assert(sessions == 0);
		assert(totalConnections == totalIdleConnections);
		vector<Connection>::iterator it, end = idleConnections.end();

		for (it = idleConnections.begin(); it != end; it++) {
			try {
				it->close();
			} catch (const SystemException &e) {
				P_ERROR("Cannot close a connection with socket " << address << ": " << e.what());
			}
		}
		idleConnections.clear();
		totalConnections = 0;
		totalIdleConnections = 0;
	}


	bool isIdle() const {
		return sessions == 0;
	}

	int busyness() const {
		/* Different sockets within a Process may have different
		 * 'concurrency' values. We want:
		 * - Process.sessionSockets to sort the sockets from least used to most used.
		 * - to give sockets with concurrency == 0 more priority over sockets
		 *   with concurrency > 0.
		 * Therefore, we describe our busyness as a percentage of 'concurrency', with
		 * the percentage value in [0..INT_MAX] instead of [0..1].
		 */
		if (concurrency == 0) {
			// Allows Process.sessionSockets to give
			// idle sockets more priority.
			if (sessions == 0) {
				return 0;
			} else {
				return 1;
			}
		} else {
			return (int) (((long long) sessions * INT_MAX) / (double) concurrency);
		}
	}

	bool isTotallyBusy() const {
		return concurrency != 0 && sessions >= concurrency;
	}

	void recreateStrings(psg_pool_t *newPool) {
		recreateString(newPool, name);
		recreateString(newPool, address);
		recreateString(newPool, protocol);
	}
};

class SocketList: public SmallVector<Socket, 1> {
public:
	void add(pid_t pid, const StaticString &name, const StaticString &address,
		const StaticString &protocol, int concurrency)
	{
		push_back(Socket(pid, name, address, protocol, concurrency));
	}

	const Socket *findSocketWithName(const StaticString &name) const {
		const_iterator it, end = this->end();
		for (it = begin(); it != end; it++) {
			if (it->name == name) {
				return &(*it);
			}
		}
		return NULL;
	}

	bool hasSessionSockets() const {
		const_iterator it;
		for (it = begin(); it != end(); it++) {
			if (it->protocol == "session" || it->protocol == "http_session") {
				return true;
			}
		}
		return false;
	}
};

typedef boost::shared_ptr<SocketList> SocketListPtr;


} // namespace ApplicationPool2
} // namespace Pasenger

#endif /* _PASSENGER_APPLICATION_POOL2_SOCKET_H_ */
