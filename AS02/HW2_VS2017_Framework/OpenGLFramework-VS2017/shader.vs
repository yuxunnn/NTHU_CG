#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vertex_pos;
out vec3 vertex_color;
out vec3 vertex_normal;

#define PI 3.14159265358979323846
/* light source */ 
uniform int lightSource;
#define DIRECTIONALLIGHT 0
#define POINTLIGHT 1
#define SPOTLIGHT 2
/* lighting mode */ 
uniform int lightingMode;
#define PERVERTEXLIGHTING 0
#define PERPIXELLIGHTING 1
/* matrix */
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
/* directional light */ 
uniform vec3 directionalLight_position;
uniform vec3 directionalLight_direction;
uniform vec3 directionalLight_ambientIntensity;
uniform vec3 directionalLight_diffuseIntensity;
uniform vec3 directionalLight_specularIntensity;
uniform float directionalLight_shininess;
/* point light */
uniform vec3 pointLight_position;
uniform vec3 pointLight_ambientIntensity;
uniform vec3 pointLight_diffuseIntensity;
uniform vec3 pointLight_specularIntensity;
uniform float pointLight_shininess;
uniform float pointLight_constant;
uniform float pointLight_linear;
uniform float pointLight_quadratic;
/* spot light */
uniform vec3 spotLight_position;
uniform vec3 spotLight_direction;
uniform float spotLight_exponent;
uniform float spotLight_cutoff;
uniform vec3 spotLight_ambientIntensity;
uniform vec3 spotLight_diffuseIntensity;
uniform vec3 spotLight_specularIntensity;
uniform float spotLight_shininess;
uniform float spotLight_constant;
uniform float spotLight_linear;
uniform float spotLight_quadratic;
/* material */
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
/* camera */
uniform vec3 camera_position;

vec3 PositionSpaceTransform(vec3 v, mat4 trans) {
	return (trans * vec4(v, 1.0)).xyz;
}
vec3 DirectionSpaceTransform(vec3 v, mat4 trans) {
	return (transpose(inverse(trans)) * vec4(v, 0)).xyz;
}

float Attenuation() {
	float value, dL;

	switch (lightSource) {
		case DIRECTIONALLIGHT:
			value = 1.0;
			break;
		case POINTLIGHT:
			dL = distance(PositionSpaceTransform(pointLight_position, viewMatrix), vertex_pos);
			value = min(1.0 / (pointLight_constant + pointLight_linear * dL + pointLight_quadratic * dL * dL), 1.0);
			break;
		case SPOTLIGHT:
			dL = distance(PositionSpaceTransform(spotLight_position, viewMatrix), vertex_pos);
			value = min(1.0 / (spotLight_constant + spotLight_linear * dL + spotLight_quadratic * dL * dL), 1.0);
			break;
		default:
			value = 1.0;
			break;
	}

	return value;
}

float SpotlightEffect() {
	float value;

	switch (lightSource) {
		case DIRECTIONALLIGHT:
			value = 1.0;
			break;
		case POINTLIGHT:
			value = 1.0;
			break;
		case SPOTLIGHT:
			vec3 v = normalize(vertex_pos - PositionSpaceTransform(spotLight_position, viewMatrix));
			vec3 d = normalize(DirectionSpaceTransform(spotLight_direction, viewMatrix));

			if (dot(v, d) < cos(spotLight_cutoff * PI / 180.0)) {
				value = 0.0;
			} 
			else {
				value = pow(max(dot(v, d), 0), spotLight_exponent);
			}
			break;
		default:
			value = 1.0;
			break;
	}

	return value;
}

vec3 Ambient(vec3 Ia) {
	return Ia * Ka;
}
vec3 Diffuse(vec3 N, vec3 L, vec3 Id) {
	return max(dot(N, L), 0.0) * Id * Kd;
}
vec3 Specular(vec3 R, vec3 V, float shininess, vec3 Is) {
	return pow(max(dot(R, V), 0.0), shininess) * Is * Ks;
}

vec3 DirectionalLight() {
	vec3 N = normalize(vertex_normal);
	vec3 L = normalize(DirectionSpaceTransform(directionalLight_position, viewMatrix));
	vec3 R = reflect(-L, N);
	vec3 V = normalize(PositionSpaceTransform(camera_position, viewMatrix) - vertex_pos);
	vec3 Ia = directionalLight_ambientIntensity;
	vec3 Id = directionalLight_diffuseIntensity;
	vec3 Is = directionalLight_specularIntensity;
	float shininess = directionalLight_shininess;

	return Attenuation() * SpotlightEffect() * (Ambient(Ia) + Diffuse(N, L, Id) + Specular(R, V, shininess, Is));
} 

vec3 PointLight() {
	vec3 N = normalize(vertex_normal);
	vec3 L = normalize(PositionSpaceTransform(pointLight_position, viewMatrix) - vertex_pos);
	vec3 R = reflect(-L, N);
	vec3 V = normalize(PositionSpaceTransform(camera_position, viewMatrix) - vertex_pos);
	vec3 Ia = pointLight_ambientIntensity;
	vec3 Id = pointLight_diffuseIntensity;
	vec3 Is = pointLight_specularIntensity;
	float shininess = pointLight_shininess;

	return Attenuation() * SpotlightEffect() * (Ambient(Ia) + Diffuse(N, L, Id) + Specular(R, V, shininess, Is));
}

vec3 SpotLight() {
	vec3 N = normalize(vertex_normal);
	vec3 L = normalize(PositionSpaceTransform(spotLight_position, viewMatrix) - vertex_pos);
	vec3 R = reflect(-L, N);
	vec3 V = normalize(PositionSpaceTransform(camera_position, viewMatrix) - vertex_pos);
	vec3 Ia = spotLight_ambientIntensity;
	vec3 Id = spotLight_diffuseIntensity;
	vec3 Is = spotLight_specularIntensity;
	float shininess = spotLight_shininess;

	return Attenuation() * SpotlightEffect() * (Ambient(Ia) + Diffuse(N, L, Id) + Specular(R, V, shininess, Is));
}

void main()
{
	// [TODO]
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPos, 1.0);

	vertex_normal = DirectionSpaceTransform(aNormal, viewMatrix * modelMatrix);
	vertex_pos = PositionSpaceTransform(aPos, viewMatrix * modelMatrix);

	if (lightingMode == PERVERTEXLIGHTING) {
		switch (lightSource) {
			case DIRECTIONALLIGHT:
				vertex_color = DirectionalLight();
				break;
			case POINTLIGHT:
				vertex_color = PointLight();
				break;
			case SPOTLIGHT:
				vertex_color = SpotLight();
				break;
			default:
				vertex_color = vec3(0.0, 0.0, 0.0);
				break;
		}
	} 
	else {
		vertex_color = aColor;
	}
}

