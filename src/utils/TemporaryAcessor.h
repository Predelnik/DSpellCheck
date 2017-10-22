#pragma once

#include <functional>

template <typename HeldType>
class TemporaryAcessor {
public:
    TemporaryAcessor(HeldType& held, std::function<void ()> finalizer) : m_held(held), m_finalizer(finalizer) {
    }

    auto &operator*() { return m_held; }
    auto &operator*() const { return m_held; }
    auto operator->() { return &m_held; }
    auto operator->() const { return &m_held; }
    ~TemporaryAcessor() { m_finalizer(); }

private:
    HeldType& m_held;
    std::function<void ()> m_finalizer;
};
