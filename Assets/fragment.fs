#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

struct MaterialParameters {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform sampler2D tex;
uniform samplerCube skybox;
uniform int state;
uniform int isTextured;
uniform MaterialParameters Material;

in vec3 tttt;
in vec3 normal;
void main()
{	
	
	vec4 ambient_D = Material.ambient;
	vec4 vv4color = ambient_D;

	if(state == 0){
		if( isTextured == 1){
			vec3 texColor = texture(tex,vertexData.texcoord).rgb;
			fragColor = vec4(texColor, 1.0);
		}else{
			fragColor = vv4color;
			//fragColor = vec4(normal, 1.0);
		}
	}else if(state == 1){
		fragColor = texture(skybox, tttt);
	}
}