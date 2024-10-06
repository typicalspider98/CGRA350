
#include "app_context.h"
#include "constants.h"


CGRA350::AppContext::AppContext() 
	: m_last_mouse_x(CGRA350Constants::DEFAULT_WINDOW_WIDTH / 2.0f),
	m_last_mouse_y(CGRA350Constants::DEFAULT_WINDOW_HEIGHT / 2.0f),
	m_render_camera(CGRA350Constants::CAMERA_POSITION, CGRA350Constants::CAMERA_RESOLUTION),
	m_gui_param()
{

}