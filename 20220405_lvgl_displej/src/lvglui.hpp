#pragma once

#include <mutex>
#include <functional>

template<typename CtxT>
class LvglUiHelper {
public:
    LvglUiHelper(CtxT ctx = CtxT()) : m_context(ctx) {

    }
    virtual ~LvglUiHelper() {}

    void modify(std::function<void(CtxT&)> cb) {
        m_ui_mutex.lock();
        cb(m_context);
        m_ui_mutex.unlock();
    }

private:
    std::mutex m_ui_mutex;
    CtxT m_context;
};
