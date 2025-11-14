#pragma once
#include <memory>
#include <string>

namespace aiecad {
class Event;

class Layer {
public:
	explicit Layer(std::string name = "Layer"): m_name(std::move(name)){};
	virtual ~Layer() = default;

	virtual void onAttach() {}
	virtual void onDetach() {}
	virtual void onUpdate(float dt) {}
	virtual void onImGuiRender() {}
	virtual void onEvent(Event& /* event */) {}

	const std::string& getName() const { return m_name; }

private:
	std::string m_name;
};
} // namespace aiecad