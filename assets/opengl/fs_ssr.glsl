#version 330

layout (location = 0) out vec3 fragColor;

uniform sampler2DArray gBufferSampler;
uniform sampler2D sceneSampler;
uniform mat4 P;

in vec2 uv;

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed, i);
	float dot_product = dot(seed4, vec4(12.9898f, 78.233f, 45.164f, 94.673f));
	return fract(sin(dot_product) * 43758.5453f);
}

void main()
{
	fragColor = texture(sceneSampler, uv).rgb;
	vec3 pos_view_space = texture(gBufferSampler, vec3(uv, 0.f)).xyz;
	vec3 normal_view_space = texture(gBufferSampler, vec3(uv, 1.f)).xyz;
	if (normal_view_space == vec3(0.f)) {
		return;
	}
	float steps = 30.f;
	vec3 ray_dir = normalize(reflect(normalize(pos_view_space), normalize(normal_view_space)));
	ray_dir += random(pos_view_space, 0) * 0.01f;
	float ray_len = -pos_view_space.z * 3.f;
	ray_len = max(ray_len, 30.f);
	vec3 ray = ray_dir * ray_len;
	vec3 delta = ray / steps;
	vec3 hit_pos = pos_view_space + delta;
	
	bool hit = false; 
	vec4 hit_pos_screen;
	for (float i = 0; i < steps; i++, hit_pos += delta) {
		hit_pos_screen = P * vec4(hit_pos, 1.f);
		hit_pos_screen /= hit_pos_screen.w;
		hit_pos_screen = hit_pos_screen * 0.5f + 0.5f;
		float depth = texture(gBufferSampler, vec3(hit_pos_screen.xy, 0.f)).z;
		if (depth > hit_pos.z) {
			hit = true;
			break;
		}
	}
	
	hit = hit && (all(greaterThan(hit_pos_screen.xyz, vec3(0.f))) && all(lessThan(hit_pos_screen.xyz, vec3(1.f))));
	
	if (hit) {
		for (float i = 0; i < 6.f; i++) { // binary search refinement			
			hit_pos_screen = P * vec4(hit_pos, 1.f);
			hit_pos_screen.xy /= hit_pos_screen.w;
			hit_pos_screen.xy = hit_pos_screen.xy * 0.5f + 0.5f;
			float depth = texture(gBufferSampler, vec3(hit_pos_screen.xy, 0.f)).z;
			delta *= 0.5f;
			float is_hit = float(hit_pos.z < depth);
			hit_pos += mix(delta, -delta, is_hit); // step back if hit, else step front
		}
	
		hit_pos_screen = P * vec4(hit_pos, 1.f);
		hit_pos_screen.xy /= hit_pos_screen.w;
		hit_pos_screen.xy = hit_pos_screen.xy * 0.5f + 0.5f;
	
		vec2 dist = min(hit_pos_screen.xy, 1.f - hit_pos_screen.xy);
		float dist_to_border = min(dist.x, dist.y);
		float factor = 1.f - smoothstep(0.f, 0.05f, dist_to_border);
		fragColor *= mix(texture(sceneSampler, hit_pos_screen.xy).rgb, vec3(1.f), factor);
	}
}