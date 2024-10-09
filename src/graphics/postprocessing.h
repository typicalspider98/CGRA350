#pragma once

#include "renderers.h"
#include "shaders.h"
#include "../ui/ui.h"

class Postprocessing : Renderer {
private:
	int m_lastScreenWidth;
	int m_lastScreenHeight;

	GLuint m_framebuffer;
	GLuint m_texColorBuffer;
	GLuint m_quadVAO, m_quadVBO;

	void prepare();
	void resize();
	void release();
	void render(const Camera& render_cam) {};

public:
	Postprocessing(ShaderProgram& shader_prog);
	Postprocessing(ShaderProgram& shader_prog, int screenWidth, int screenHeight);
	~Postprocessing() { release(); }

	void beforeRender(int screenWidth, int screenHeight);

	void render(GUIParam& param);
	void renderQuad();
};