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
#ifndef _PASSENGER_APPLICATION_POOL2_DUMMY_SPAWNER_H_
#define _PASSENGER_APPLICATION_POOL2_DUMMY_SPAWNER_H_

#include <ApplicationPool2/Spawner.h>

namespace Passenger {
namespace ApplicationPool2 {

using namespace std;
using namespace boost;
using namespace oxt;


class DummySpawner: public Spawner {
private:
	boost::mutex lock;
	unsigned int count;

public:
	unsigned int cleanCount;

	DummySpawner(const SpawnerConfigPtr &_config)
		: Spawner(_config)
	{
		count = 0;
		cleanCount = 0;
	}

	virtual SpawnObject spawn(const Options &options) {
		TRACE_POINT();
		possiblyRaiseInternalError(options);

		pid_t pid;
		{
			boost::lock_guard<boost::mutex> l(lock);
			count++;
			pid = count;
		}

		SocketPair adminSocket = createUnixSocketPair(__FILE__, __LINE__);
		SocketList sockets;
		sockets.add(pid, "main", "tcp://127.0.0.1:1234", "session", config->concurrency);
		syscalls::usleep(config->spawnTime);

		SpawnObject object;
		string gupid = "gupid-" + toString(pid);
		object.process = boost::make_shared<Process>(
			pid, gupid,
			adminSocket.second, FileDescriptor(), sockets,
			SystemTime::getUsec(), SystemTime::getUsec());
		object.process->dummy = true;
		return boost::move(object);
	}

	virtual bool cleanable() const {
		return true;
	}

	virtual void cleanup() {
		cleanCount++;
	}
};

typedef boost::shared_ptr<DummySpawner> DummySpawnerPtr;


} // namespace ApplicationPool2
} // namespace Passenger

#endif /* _PASSENGER_APPLICATION_POOL2_DUMMY_SPAWNER_H_ */
