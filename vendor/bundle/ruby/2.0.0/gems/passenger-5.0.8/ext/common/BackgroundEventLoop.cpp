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
#include <cstdlib>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <oxt/thread.hpp>
#include <ev++.h>
#include <eio.h>
#include <BackgroundEventLoop.h>
#include <Logging.h>
#include <Exceptions.h>
#include <SafeLibev.h>

namespace Passenger {

using namespace std;
using namespace boost;
using namespace oxt;


struct BackgroundEventLoopPrivate {
	oxt::thread *thr;
	boost::mutex lock;
	boost::condition_variable cond;
	ev_idle eioRepeatWatcher;
	ev_async eioReadyWatcher;
	bool useLibeio;
	bool started;
};

static BackgroundEventLoop *eioInstanceData = NULL;


static void
signalBackgroundEventLoopExit(struct ev_loop *loop, ev_async *async, int revents) {
	BackgroundEventLoop *bg = (BackgroundEventLoop *) async->data;
	ev_break(bg->loop, EVBREAK_ALL);
}

static void
startBackgroundLoop(BackgroundEventLoop *bg) {
	boost::unique_lock<boost::mutex> l(bg->priv->lock);
	bg->safe->setCurrentThread();
	bg->priv->started = true;
	bg->priv->cond.notify_all();
	l.unlock();
	ev_run(bg->loop, 0);
}

static void
eioRepeat(EV_P_ ev_idle *w, int revents) {
	if (eio_poll() != -1) {
		ev_idle_stop(EV_A_ w);
	}
}

/* eio has some results, process them */
static void
eioReady(EV_P_ ev_async *w, int revents) {
	if (eio_poll() == -1) {
		ev_idle_start(EV_A_ &eioInstanceData->priv->eioRepeatWatcher);
	}
}

/* wake up the event loop */
static void
eioWantPoll(void) {
	ev_async_send(eioInstanceData->loop, &eioInstanceData->priv->eioReadyWatcher);
}

BackgroundEventLoop::BackgroundEventLoop(bool scalable, bool useLibeio) {
	TRACE_POINT();

	if (scalable) {
		loop = ev_loop_new(EVBACKEND_KQUEUE);
		if (loop == NULL) {
			loop = ev_loop_new(EVBACKEND_EPOLL);
		}
		if (loop == NULL) {
			loop = ev_loop_new(EVFLAG_AUTO);
		}
	} else {
		loop = ev_loop_new(EVBACKEND_POLL);
	}
	if (loop == NULL) {
		throw RuntimeException("Cannot create an event loop");
	}

	P_LOG_FILE_DESCRIPTOR_OPEN2(ev_backend_fd(loop), "libev event loop: backend FD");

	async = (ev_async *) malloc(sizeof(ev_async));
	async->data = this;
	ev_async_init(async, signalBackgroundEventLoopExit);
	ev_async_start(loop, async);
	P_LOG_FILE_DESCRIPTOR_OPEN2(ev_loop_get_pipe(loop, 0), "libev event loop: async pipe 0");
	P_LOG_FILE_DESCRIPTOR_OPEN2(ev_loop_get_pipe(loop, 1), "libev event loop: async pipe 1");
	safe = boost::make_shared<SafeLibev>(loop);
	priv = new BackgroundEventLoopPrivate();
	priv->thr = NULL;
	priv->useLibeio = useLibeio;
	priv->started = false;

	if (useLibeio) {
		eioInstanceData = this;
		ev_idle_init(&priv->eioRepeatWatcher, eioRepeat);
		ev_async_init(&priv->eioReadyWatcher, eioReady);
		ev_async_start(loop, &priv->eioReadyWatcher);
		eio_init(eioWantPoll, 0);
	}
}

BackgroundEventLoop::~BackgroundEventLoop() {
	stop();
	if (priv->useLibeio) {
		ev_idle_stop(loop, &priv->eioRepeatWatcher);
		ev_async_stop(loop, &priv->eioReadyWatcher);
		eioInstanceData = NULL;
	}
	ev_async_stop(loop, async);
	delete priv;
	free(async);
}

void
BackgroundEventLoop::start(const string &threadName, unsigned int stackSize) {
	assert(priv->thr == NULL);
	boost::unique_lock<boost::mutex> l(priv->lock);
	priv->thr = new oxt::thread(
		boost::bind(startBackgroundLoop, this),
		threadName,
		stackSize
	);
	while (!priv->started) {
		priv->cond.wait(l);
	}
}

void
BackgroundEventLoop::stop() {
	if (priv->thr != NULL) {
		ev_async_send(loop, async);
		priv->thr->join();
		delete priv->thr;
		priv->thr = NULL;
	}
}

bool
BackgroundEventLoop::isStarted() const {
	return priv->thr != NULL;
}

pthread_t
BackgroundEventLoop::getNativeHandle() const {
	return priv->thr->native_handle();
}


} // namespace Passenger
