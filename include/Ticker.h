// https://github.com/ozooma10/OSLAroused-SKSE/blob/master/src/Utilities/Ticker.h
class Ticker {
public:
    Ticker(std::function<void()> onTick, std::chrono::milliseconds interval)
        : m_OnTick(onTick), m_Interval(interval), m_Running(false), m_ThreadActive(false) {}

    void Start();

    inline void Stop() { m_Running = false; }

    void UpdateInterval(std::chrono::milliseconds newInterval);

private:
    void RunLoop();

    std::function<void()> m_OnTick;
    std::chrono::milliseconds m_Interval;

    std::atomic<bool> m_ThreadActive;
    std::atomic<bool> m_Running;
    std::mutex m_IntervalMutex;
};