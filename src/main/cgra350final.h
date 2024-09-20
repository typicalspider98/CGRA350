#ifndef CGRA350_MAIN
#define CGRA350_MAIN
#pragma once

#include "../graphics/window.h"
#include "app_context.h"
#include "../ui/ui.h"


namespace CGRA350
{
	class CGRA350App
	{
	private:
		Window m_window;
		AppContext m_context;

	public:

		CGRA350App();	// init
		~CGRA350App();	// terminate

		static void initGLFW();
		void renderLoop();
	};
}
#endif