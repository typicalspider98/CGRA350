#include "postprocessing.h"
#include <iostream>

void Postprocessing::prepare()
{
	// create framebuffer
	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	// create texture
	glGenTextures(1, &m_texColorBuffer);
	glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_POSTPROCESSING);
	glBindTexture(GL_TEXTURE_2D, m_texColorBuffer);

	// set texture parameters
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_lastScreenWidth, m_lastScreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// add texture to frame buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texColorBuffer, 0);

	// check
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer is not complete!" << std::endl;

	// unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_shader_prog.use();
	m_shader_prog.setInt("scene", CGRA350Constants::TEX_SAMPLE_ID_POSTPROCESSING);
	m_shader_prog.use_end();
}

void Postprocessing::resize()
{
	if (m_texColorBuffer != 0)
	{
		glDeleteTextures(1, &m_texColorBuffer);
		m_texColorBuffer = 0;
	}

	// bind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	// create texture
	glGenTextures(1, &m_texColorBuffer);
	glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_POSTPROCESSING);
	glBindTexture(GL_TEXTURE_2D, m_texColorBuffer);

	// set texture parameters
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_lastScreenWidth, m_lastScreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// add texture to frame buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texColorBuffer, 0);

	// unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Postprocessing::release()
{
	// check
	if (m_framebuffer != 0)
	{
		glDeleteFramebuffers(1, &m_framebuffer);
		m_framebuffer = 0;
	}

	if (m_texColorBuffer != 0)
	{
		glDeleteTextures(1, &m_texColorBuffer);
		m_texColorBuffer = 0;
	}

	if (m_quadVAO != 0)
	{
		glDeleteVertexArrays(1, &m_quadVAO);
		m_quadVAO = 0;
	}

	if (m_quadVBO != 0)
	{
		glDeleteBuffers(1, &m_quadVBO);
		m_quadVBO = 0;
	}
}

Postprocessing::Postprocessing(ShaderProgram& shader_prog) : Renderer(shader_prog), m_lastScreenWidth(0), m_lastScreenHeight(0), 
															 m_framebuffer(0), m_texColorBuffer(0), m_quadVAO(0), m_quadVBO(0)
{
}

Postprocessing::Postprocessing(ShaderProgram& shader_prog, int screenWidth, int screenHeight) : Renderer(shader_prog), 
															m_lastScreenWidth(screenWidth), m_lastScreenHeight(screenHeight), 
															m_framebuffer(0), m_texColorBuffer(0), m_quadVAO(0), m_quadVBO(0)
{
	this->prepare();
}

void Postprocessing::beforeRender(int screenWidth, int screenHeight)
{
	if (screenWidth != m_lastScreenWidth || screenHeight != m_lastScreenHeight)
	{
		this->m_lastScreenWidth = screenWidth;
		this->m_lastScreenHeight = screenHeight;

		this->resize();
	}

	// bind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, this->m_framebuffer);
}

void Postprocessing::render(GUIParam& param)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	m_shader_prog.use();
	glActiveTexture(GL_TEXTURE0 + CGRA350Constants::TEX_SAMPLE_ID_POSTPROCESSING);
	glBindTexture(GL_TEXTURE_2D, m_texColorBuffer);
	m_shader_prog.setInt("scene", CGRA350Constants::TEX_SAMPLE_ID_POSTPROCESSING);

	// pass Color Grading parameters
	m_shader_prog.setInt("enableColorGrading", param.enableColorGrading);
	m_shader_prog.setVec3("colorFilter", param.colorFilter);

	// pass Dynamic Filter parameters
	m_shader_prog.setInt("enableDynamicFilter", param.enableDynamicFilter);
	m_shader_prog.setFloat("contrast", param.contrast);
	m_shader_prog.setFloat("brightness", param.brightness);

	// pass Gaussian Blur parameters
	m_shader_prog.setInt("enableGaussianBlur", param.enableGaussianBlur);
	m_shader_prog.setVec2("blurDirection", param.blurDirection);
	m_shader_prog.setFloatArray("blurWeight", param.blurWeight, 5);
	m_shader_prog.setVec2Array("blurOffset", param.blurOffset, 5);

	// pass Bloom Effect parameters
	m_shader_prog.setInt("enableBloom", param.enableBloom);
	m_shader_prog.setFloat("bloomThreshold", param.bloomThreshold);
	m_shader_prog.setFloat("bloomIntensity", param.bloomIntensity);

	renderQuad();

	m_shader_prog.use_end();
}

void Postprocessing::renderQuad()
{
	if (m_quadVAO == 0)
	{
		float quadVertices[] = {
			// position      // uv
			-1.0f,  1.0f,    0.0f, 1.0f,
			-1.0f, -1.0f,    0.0f, 0.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,

			-1.0f,  1.0f,    0.0f, 1.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,
			 1.0f,  1.0f,    1.0f, 1.0f
		};
		glGenVertexArrays(1, &m_quadVAO);
		glGenBuffers(1, &m_quadVBO);
		glBindVertexArray(m_quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	}

	glBindVertexArray(m_quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

