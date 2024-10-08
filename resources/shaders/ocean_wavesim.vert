#version 430 core

layout (location = 0) in vec3 oc_pos;
layout (location = 1) in vec2 tex_coords;

uniform float some_scale;  // 定义缩放因子
// A basic 2D Perlin noise implementation
vec2 hash(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float perlin_noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(dot(hash(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
                   dot(hash(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
               mix(dot(hash(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
                   dot(hash(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x), u.y);
}

out VS_OUT
{
	vec3 wc_pos;
	vec3 wc_normal;
	vec2 tex_coords;
} vs_out;

// transformation matrices & time
uniform mat4 m_matrix;
uniform mat4 vp_matrix;
uniform float time;

// wave simulation parameters
const int NUM_WAVES = 16;
uniform vec2 sim_wavevecs[NUM_WAVES];
uniform float sim_freqs[NUM_WAVES];
uniform float sim_amplitudes[NUM_WAVES];
uniform float sim_phases[NUM_WAVES];

// heightmap functions
float heightmap(vec2 p);		// gives the value of the heightmap at point (x,z)
float heightmap_dx(vec2 p);		// gives the value of the derivative in the x direction, at pt (x,z)
float heightmap_dz(vec2 p);		// like ^^ but for z direction

// horizontal displacementent functions
vec2 horiz_displ(vec2 p);		// gives the horizontal displacement
vec2 horiz_displ_dx(vec2 p);	// gives value of the derivative in the x direction, at pt (x,z)
vec2 horiz_displ_dz(vec2 p);	// gives value of the derivative in the x direction, at pt (x,z)


void main()
{
	vec4 wc_pos = m_matrix * vec4(oc_pos, 1.0);
	vs_out.wc_pos = vec3(wc_pos);

	// calc displaced position
	vec2 horiz_change = horiz_displ(wc_pos.xz);
	vec4 wc_pos_displaced = vec4(	
		wc_pos.x - horiz_change.x, 
		wc_pos.y + heightmap(wc_pos.xz), 
		wc_pos.z - horiz_change.y,
		wc_pos.w
	);

	// calc derivatives
	vec2 horiz_change_dx = horiz_displ_dx(wc_pos.xz);
	vec2 horiz_change_dz = horiz_displ_dz(wc_pos.xz);
	vec3 wc_pos_displaced_dx = vec3(1-horiz_change_dx.x, heightmap_dx(wc_pos.xz), horiz_change_dx.y);
	vec3 wc_pos_displaced_dz = vec3(horiz_change_dz.x, heightmap_dz(wc_pos.xz), 1-horiz_change_dz.y);
	
	// outputs
	gl_Position = vp_matrix * wc_pos_displaced;
	vs_out.wc_normal = cross(wc_pos_displaced_dx, wc_pos_displaced_dz);
	vs_out.tex_coords = tex_coords;
}

float heightmap(vec2 p)
{
#if 0
	float res = 0;
	for (int i = 0; i < NUM_WAVES; i++)
	{
		res += sim_amplitudes[i] * cos(dot(sim_wavevecs[i], p) - sim_freqs[i] * time + sim_phases[i]);
	}

	return res; 
#endif
	float res = 0;
    for (int i = 0; i < NUM_WAVES; i++)
    {
        // 使用更复杂的波谱模型替代正弦波
        res += sim_amplitudes[i] * cos(dot(sim_wavevecs[i], p) - sim_freqs[i] * time + sim_phases[i]);
    }

    // 叠加一个噪声函数来增加细节
    res += 0.1 * perlin_noise(p * some_scale + time);
    
    return res; 
}

float heightmap_dx(vec2 p)
{
	float res = 0;
	for (int i = 0; i < NUM_WAVES; i++)
	{
		res += sim_amplitudes[i] * sim_wavevecs[i].x * -sin(dot(sim_wavevecs[i], p) - sim_freqs[i] * time + sim_phases[i]);
	}
	return res;
}

float heightmap_dz(vec2 p)
{
	float res = 0;
	for (int i = 0; i < NUM_WAVES; i++)
	{
		res += sim_amplitudes[i] * sim_wavevecs[i].y * -sin(dot(sim_wavevecs[i], p) - sim_freqs[i] * time + sim_phases[i]);
	}
	return res;
}

vec2 horiz_displ(vec2 p)
{
	vec2 res = vec2(0);
	for (int i = 0; i < NUM_WAVES; i++)
	{
		res += normalize(sim_wavevecs[i]) * sim_amplitudes[i] * sin(dot(sim_wavevecs[i], p) - sim_freqs[i] * time + sim_phases[i]);
	}
	return res;
}

vec2 horiz_displ_dx(vec2 p)
{
	vec2 res = vec2(0);
	for (int i = 0; i < NUM_WAVES; i++)
	{
		res += normalize(sim_wavevecs[i]) * sim_amplitudes[i] * sim_wavevecs[i].x * sin(dot(sim_wavevecs[i], p) - sim_freqs[i] * time + sim_phases[i]);
	}
	return res;
}

vec2 horiz_displ_dz(vec2 p)
{
	vec2 res = vec2(0);
	for (int i = 0; i < NUM_WAVES; i++)
	{
		res += normalize(sim_wavevecs[i]) * sim_amplitudes[i] * sim_wavevecs[i].y * sin(dot(sim_wavevecs[i], p) - sim_freqs[i] * time + sim_phases[i]);
	}
	return res;
}
