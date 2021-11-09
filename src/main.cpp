#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include "render/window.hpp"
#include "app/main_app.hpp"

#include "config.hpp"

#include <future>
#include <thread>

int main(int argc, char *argv[])
{
	spdlog::cfg::load_env_levels();
#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#endif
	spdlog::info("Welcome to your playground!");

	render::window window;
	window.init();

	window.set_phase(new app::main_phase(&window));

	window.loop();

	return 0;
}