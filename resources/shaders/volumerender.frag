#version 330

in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D TexSampler;
uniform int Size;
uniform int FSR;
uniform float sharp;

void FsrEASU(out vec4 fragColor)
{    
	vec2 fragCoord = TexCoord * Size * 2;
	vec4 scale = vec4(1. / vec2(Size), vec2(0.5f));
	vec2 src_pos = scale.zw * fragCoord;
	vec2 src_centre = floor(src_pos - .5) + .5;

	vec4 f; 
	f.zw = 1. - (f.xy = src_pos - src_centre);
	vec4 l2_w0_o3 = ((1.5672 * f - 2.6445) * f + 0.0837) * f + 0.9976;
	vec4 l2_w1_o3 = ((-0.7389 * f + 1.3652) * f - 0.6295) * f - 0.0004;
	vec4 w1_2 = l2_w0_o3;
	vec2 w12 = w1_2.xy + w1_2.zw;
	vec4 wedge = l2_w1_o3.xyzw * w12.yxyx;
	vec2 tc12 = scale.xy * (src_centre + w1_2.zw / w12);
	vec2 tc0 = scale.xy * (src_centre - 1.);
	vec2 tc3 = scale.xy * (src_centre + 2.);
	vec3 col = vec3(texture(TexSampler, vec2(tc12.x, tc0.y)).rgb * wedge.y
	         + texture(TexSampler, vec2(tc0.x, tc12.y)).rgb * wedge.x
			 + texture(TexSampler, tc12.xy).rgb * (w12.x * w12.y)
			 + texture(TexSampler, vec2(tc3.x, tc12.y)).rgb * wedge.z
			 + texture(TexSampler, vec2(tc12.x, tc3.y)).rgb * wedge.w);

	fragColor = vec4(col,1);
}

void FsrRCAS(float sharp, out vec4 fragColor)
{    
	vec2 uv = TexCoord;
	vec3 col = texture(TexSampler, uv).xyz;
	float max_g = col.y;
	float min_g = col.y;
	vec4 uvoff = vec4(1,0,1,-1)/Size;

	vec3 colw;
	vec3 col1 = texture(TexSampler, uv+uvoff.yw).xyz;
	max_g = max(max_g, col1.y);
	min_g = min(min_g, col1.y);
	colw = col1;

	col1 = texture(TexSampler, uv+uvoff.xy).xyz;
	max_g = max(max_g, col1.y);
	min_g = min(min_g, col1.y);
	colw += col1;

	col1 = texture(TexSampler, uv+uvoff.yz).xyz;
	max_g = max(max_g, col1.y);
	min_g = min(min_g, col1.y);
	colw += col1;
	
	col1 = texture(TexSampler, uv-uvoff.xy).xyz;
	max_g = max(max_g, col1.y);
	min_g = min(min_g, col1.y);
	colw += col1;

	float d_min_g = min_g;
	float d_max_g = 1.-max_g;
	float A;
	max_g = max(0., max_g);
	if (d_max_g < d_min_g) 
	{        
		A = d_max_g / max_g;
	} 
	else 
	{   
		A = d_min_g / max_g;    
	}
	A = sqrt(max(0., A));
	A *= mix(-.125, -.2, sharp);
	
	vec3 col_out = (col + colw * A) / (1.+4.*A);
	fragColor = vec4(col_out,1);
}

void main() {
    if (FSR == 1) 
		FsrEASU(FragColor);
    else if (FSR == 2) 
		FsrRCAS(sharp, FragColor);
    else 
	{	
		vec4 texColor = texture(TexSampler, TexCoord);
		float alpha = texColor.a;
		FragColor = vec4(texColor.rgb, alpha);
	}
}