#include "Ticker.h"

void Ticker::Start()
{
	if (m_Running) {
        return;
    }
    m_Running = true;
    // logger::info("Start Called with thread active state of: {}", m_ThreadActive);
    if (!m_ThreadActive) {
        std::thread tickerThread(&Ticker::RunLoop, this);
        tickerThread.detach();
    }
}

void Ticker::UpdateInterval(std::chrono::milliseconds newInterval)
{
    m_IntervalMutex.lock();
    m_Interval = newInterval;
    m_IntervalMutex.unlock();
}

void Ticker::RunLoop()
{
    m_ThreadActive = true;
    while (m_Running) {
        std::thread runnerThread(m_OnTick);
        runnerThread.detach();

        m_IntervalMutex.lock();
        std::chrono::milliseconds interval;
        if (m_Interval >= std::chrono::milliseconds(3000)) {
            interval = m_Interval;
		} else {
            interval = std::chrono::milliseconds(3000);
        }
        m_IntervalMutex.unlock();
        std::this_thread::sleep_for(interval);
    }
    m_ThreadActive = false;
}
