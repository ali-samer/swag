#pragma once

#include <cstddef>
#include <typeindex>

namespace aiecad {
class EventBus;

class EventSubscription {
public:
	EventSubscription() = default;

	EventSubscription(EventBus *      bus,
	                  std::type_index type,
	                  std::size_t     listenerId) noexcept
		: m_bus(bus), m_type(type), m_listenerId(listenerId) {
	}

	EventSubscription& operator=(EventSubscription &&other) noexcept {
		if (this != &other) {
			reset();
			m_bus              = other.m_bus;
			m_type             = other.m_type;
			m_listenerId       = other.m_listenerId;
			other.m_bus        = nullptr;
			other.m_listenerId = 0;
		}
		return *this;
	}

	~EventSubscription() { reset(); }

	void reset();
	bool isValid() const noexcept {
		return m_bus != nullptr && m_listenerId != 0;
	}

	explicit operator bool() const noexcept { return isValid(); }

private:
	EventBus *      m_bus{ nullptr };
	std::type_index m_type{ typeid(void) };
	std::size_t     m_listenerId{ 0 };
};
} // namespace aiecad
