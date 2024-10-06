#ifndef CGRA350_MAIN
#define CGRA350_MAIN
#pragma once

#include "../volumerendering/volume_gui.hpp"

#include "../graphics/window.h"
#include "app_context.h"
#include "../ui/ui.h"

#include "../volumerendering/volume.hpp"

namespace CGRA350
{
	class CGRA350App
	{
	private:
		Window m_window;
		AppContext m_context;
		VolumeRender* m_volumerender;

	public:

		CGRA350App();	// init
		~CGRA350App();	// terminate

		static void initGLFW();
		void initVolumeRendering();
		void renderLoop();
	};
}
#endif